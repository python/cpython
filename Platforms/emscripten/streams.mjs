/**
 * This is a pared down version of
 * https://github.com/pyodide/pyodide/blob/main/src/js/streams.ts
 *
 * It replaces the standard streams devices that Emscripten provides with our
 * own better ones. It fixes the following deficiencies:
 *
 * 1. The emscripten std streams always have isatty set to true. These set
 *    isatty to match the value for the stdin/stdout/stderr that node sees.
 * 2. The emscripten std streams don't support the ttygetwinsize ioctl. If
 *    isatty() returns true, then these do, and it returns the actual window
 *    size as the OS reports it to Node.
 * 3. The emscripten std streams introduce an extra layer of buffering which has
 *    to be flushed with fsync().
 * 4. The emscripten std streams are slow and complex because they go through a
 *    character-based handler layer. This is particularly awkward because both
 *    sides of this character based layer deal with buffers and so we need
 *    complex adaptors, buffering, etc on both sides. Removing this
 *    character-based middle layer makes everything better.
 *    https://github.com/emscripten-core/emscripten/blob/1aa7fb531f11e11e7ae49b75a24e1a8fe6fa4a7d/src/lib/libtty.js?plain=1#L104-L114
 *
 * Ideally some version of this should go upstream to Emscripten since it is not
 * in any way specific to Python. But I (Hood) haven't gotten around to it yet.
 */

import * as tty from "node:tty";
import * as fs from "node:fs";

let FS;
const DEVOPS = {};
const DEVS = {};

function isErrnoError(e) {
  return e && typeof e === "object" && "errno" in e;
}

const waitBuffer = new Int32Array(
  new WebAssembly.Memory({ shared: true, initial: 1, maximum: 1 }).buffer,
);
function syncSleep(timeout) {
  try {
    Atomics.wait(waitBuffer, 0, 0, timeout);
    return true;
  } catch (_) {
    return false;
  }
}

/**
 * Calls the callback and handle node EAGAIN errors.
 */
function handleEAGAIN(cb) {
  while (true) {
    try {
      return cb();
    } catch (e) {
      if (e && e.code === "EAGAIN") {
        // Presumably this means we're in node and tried to read from/write to
        // an O_NONBLOCK file descriptor. Synchronously sleep for 10ms then try
        // again. In case for some reason we fail to sleep, propagate the error
        // (it will turn into an EOFError).
        if (syncSleep(10)) {
          continue;
        }
      }
      throw e;
    }
  }
}

function readWriteHelper(stream, cb, method) {
  let nbytes;
  try {
    nbytes = handleEAGAIN(cb);
  } catch (e) {
    if (e && e.code && Module.ERRNO_CODES[e.code]) {
      throw new FS.ErrnoError(Module.ERRNO_CODES[e.code]);
    }
    if (isErrnoError(e)) {
      // the handler set an errno, propagate it
      throw e;
    }
    console.error("Error thrown in read:");
    console.error(e);
    throw new FS.ErrnoError(Module.ERRNO_CODES.EIO);
  }
  if (nbytes === undefined) {
    // Prevent an infinite loop caused by incorrect code that doesn't return a
    // value.
    // Maybe we should set nbytes = buffer.length here instead?
    console.warn(
      `${method} returned undefined; a correct implementation must return a number`,
    );
    throw new FS.ErrnoError(Module.ERRNO_CODES.EIO);
  }
  if (nbytes !== 0) {
    stream.node.timestamp = Date.now();
  }
  return nbytes;
}

function asUint8Array(arg) {
  if (ArrayBuffer.isView(arg)) {
    return new Uint8Array(arg.buffer, arg.byteOffset, arg.byteLength);
  } else {
    return new Uint8Array(arg);
  }
}

const prepareBuffer = (buffer, offset, length) =>
  asUint8Array(buffer).subarray(offset, offset + length);

const TTY_OPS = {
  ioctl_tiocgwinsz(tty) {
    return tty.devops.ioctl_tiocgwinsz?.();
  },
};

const stream_ops = {
  open: function (stream) {
    const devops = DEVOPS[stream.node.rdev];
    if (!devops) {
      throw new FS.ErrnoError(Module.ERRNO_CODES.ENODEV);
    }
    stream.devops = devops;
    stream.tty = stream.devops.isatty ? { ops: TTY_OPS, devops } : undefined;
    stream.seekable = false;
  },
  close: function (stream) {
    // flush any pending line data
    stream.stream_ops.fsync(stream);
  },
  fsync: function (stream) {
    const ops = stream.devops;
    if (ops.fsync) {
      ops.fsync();
    }
  },
  read: function (stream, buffer, offset, length, pos /* ignored */) {
    buffer = prepareBuffer(buffer, offset, length);
    return readWriteHelper(stream, () => stream.devops.read(buffer), "read");
  },
  write: function (stream, buffer, offset, length, pos /* ignored */) {
    buffer = prepareBuffer(buffer, offset, length);
    return readWriteHelper(stream, () => stream.devops.write(buffer), "write");
  },
};

function nodeFsync(fd) {
  try {
    fs.fsyncSync(fd);
  } catch (e) {
    if (e?.code === "EINVAL") {
      return;
    }
    // On Mac, calling fsync when not isatty returns ENOTSUP
    // On Windows, stdin/stdout/stderr may be closed, returning EBADF or EPERM
    if (
      e?.code === "ENOTSUP" || e?.code === "EBADF" || e?.code === "EPERM"
    ) {
      return;
    }

    throw e;
  }
}

class NodeReader {
  constructor(nodeStream) {
    this.nodeStream = nodeStream;
    this.isatty = tty.isatty(nodeStream.fd);
  }

  read(buffer) {
    try {
      return fs.readSync(this.nodeStream.fd, buffer);
    } catch (e) {
      // Platform differences: on Windows, reading EOF throws an exception,
      // but on other OSes, reading EOF returns 0. Uniformize behavior by
      // catching the EOF exception and returning 0.
      if (e.toString().includes("EOF")) {
        return 0;
      }
      throw e;
    }
  }

  fsync() {
    nodeFsync(this.nodeStream.fd);
  }
}

class NodeWriter {
  constructor(nodeStream) {
    this.nodeStream = nodeStream;
    this.isatty = tty.isatty(nodeStream.fd);
  }

  write(buffer) {
    return fs.writeSync(this.nodeStream.fd, buffer);
  }

  fsync() {
    nodeFsync(this.nodeStream.fd);
  }

  ioctl_tiocgwinsz() {
    return [this.nodeStream.rows ?? 24, this.nodeStream.columns ?? 80];
  }
}

export function initializeStreams(fsarg) {
  FS = fsarg;
  const major = FS.createDevice.major++;
  DEVS.stdin = FS.makedev(major, 0);
  DEVS.stdout = FS.makedev(major, 1);
  DEVS.stderr = FS.makedev(major, 2);

  FS.registerDevice(DEVS.stdin, stream_ops);
  FS.registerDevice(DEVS.stdout, stream_ops);
  FS.registerDevice(DEVS.stderr, stream_ops);

  FS.unlink("/dev/stdin");
  FS.unlink("/dev/stdout");
  FS.unlink("/dev/stderr");

  FS.mkdev("/dev/stdin", DEVS.stdin);
  FS.mkdev("/dev/stdout", DEVS.stdout);
  FS.mkdev("/dev/stderr", DEVS.stderr);

  DEVOPS[DEVS.stdin] = new NodeReader(process.stdin);
  DEVOPS[DEVS.stdout] = new NodeWriter(process.stdout);
  DEVOPS[DEVS.stderr] = new NodeWriter(process.stderr);

  FS.closeStream(0 /* stdin */);
  FS.closeStream(1 /* stdout */);
  FS.closeStream(2 /* stderr */);
  FS.open("/dev/stdin", 0 /* O_RDONLY */);
  FS.open("/dev/stdout", 1 /* O_WRONLY */);
  FS.open("/dev/stderr", 1 /* O_WRONLY */);
}
