// Much of this is adapted from here:
// https://github.com/mame/xterm-pty/blob/main/emscripten-pty.js
// Thanks to xterm-pty for making this possible!

import createEmscriptenModule from "./python.mjs";
import { openpty } from "https://unpkg.com/xterm-pty/index.mjs";
import "https://unpkg.com/@xterm/xterm/lib/xterm.js";

var term = new Terminal();
term.open(document.getElementById("terminal"));
const { master, slave: PTY } = openpty();
term.loadAddon(master);
globalThis.PTY = PTY;

async function setupStdlib(Module) {
  const versionInt = Module.HEAPU32[Module._Py_Version >>> 2];
  const major = (versionInt >>> 24) & 0xff;
  const minor = (versionInt >>> 16) & 0xff;
  // Prevent complaints about not finding exec-prefix by making a lib-dynload directory
  Module.FS.mkdirTree(`/lib/python${major}.${minor}/lib-dynload/`);
  const resp = await fetch(`python${major}.${minor}.zip`);
  const stdlibBuffer = await resp.arrayBuffer();
  Module.FS.writeFile(
    `/lib/python${major}${minor}.zip`,
    new Uint8Array(stdlibBuffer),
    { canOwn: true },
  );
}

const tty_ops = {
  ioctl_tcgets: () => {
    const termios = PTY.ioctl("TCGETS");
    const data = {
      c_iflag: termios.iflag,
      c_oflag: termios.oflag,
      c_cflag: termios.cflag,
      c_lflag: termios.lflag,
      c_cc: termios.cc,
    };
    return data;
  },

  ioctl_tcsets: (_tty, _optional_actions, data) => {
    PTY.ioctl("TCSETS", {
      iflag: data.c_iflag,
      oflag: data.c_oflag,
      cflag: data.c_cflag,
      lflag: data.c_lflag,
      cc: data.c_cc,
    });
    return 0;
  },

  ioctl_tiocgwinsz: () => PTY.ioctl("TIOCGWINSZ").reverse(),

  get_char: () => {
    throw new Error("Should not happen");
  },
  put_char: () => {
    throw new Error("Should not happen");
  },

  fsync: () => {},
};

const POLLIN = 1;
const POLLOUT = 4;

const waitResult = {
  READY: 0,
  SIGNAL: 1,
  TIMEOUT: 2,
};

function onReadable() {
  var handle;
  var promise = new Promise((resolve) => {
    handle = PTY.onReadable(() => resolve(waitResult.READY));
  });
  return [promise, handle];
}

function onSignal() {
  // TODO: signal handling
  var handle = { dispose() {} };
  var promise = new Promise((resolve) => {});
  return [promise, handle];
}

function onTimeout(timeout) {
  var id;
  var promise = new Promise((resolve) => {
    if (timeout > 0) {
      id = setTimeout(resolve, timeout, waitResult.TIMEOUT);
    }
  });
  var handle = {
    dispose() {
      if (id) {
        clearTimeout(id);
      }
    },
  };
  return [promise, handle];
}

async function waitForReadable(timeout) {
  let p1, p2, p3;
  let h1, h2, h3;
  try {
    [p1, h1] = onReadable();
    [p2, h2] = onTimeout(timeout);
    [p3, h3] = onSignal();
    return await Promise.race([p1, p2, p3]);
  } finally {
    h1.dispose();
    h2.dispose();
    h3.dispose();
  }
}

const FIONREAD = 0x541b;

const tty_stream_ops = {
  async readAsync(stream, buffer, offset, length, pos /* ignored */) {
    let readBytes = PTY.read(length);
    if (length && !readBytes.length) {
      const status = await waitForReadable(-1);
      if (status === waitResult.READY) {
        readBytes = PTY.read(length);
      } else {
        throw new Error("Not implemented");
      }
    }
    buffer.set(readBytes, offset);
    return readBytes.length;
  },

  write: (stream, buffer, offset, length) => {
    // Note: default `buffer` is for some reason `HEAP8` (signed), while we want unsigned `HEAPU8`.
    buffer = new Uint8Array(
      buffer.buffer,
      buffer.byteOffset,
      buffer.byteLength,
    );
    const toWrite = Array.from(buffer.subarray(offset, offset + length));
    PTY.write(toWrite);
    return length;
  },

  async pollAsync(stream, timeout) {
    if (!PTY.readable && timeout) {
      await waitForReadable(timeout);
    }
    return (PTY.readable ? POLLIN : 0) | (PTY.writable ? POLLOUT : 0);
  },
  ioctl(stream, request, varargs) {
    if (request === FIONREAD) {
      const res = PTY.fromLdiscToUpperBuffer.length;
      Module.HEAPU32[varargs / 4] = res;
      return 0;
    }
    throw new Error("Unimplemented ioctl request");
  },
};

async function setupStdio(Module) {
  Object.assign(Module.TTY.default_tty_ops, tty_ops);
  Object.assign(Module.TTY.stream_ops, tty_stream_ops);
}

const emscriptenSettings = {
  async preRun(Module) {
    Module.addRunDependency("pre-run");
    Module.ENV.TERM = "xterm-256color";
    // Uncomment next line to turn on tracing (messages go to browser console).
    // Module.ENV.PYREPL_TRACE = "1";

    // Leak module so we can try to show traceback if we crash on startup
    globalThis.Module = Module;
    await Promise.all([setupStdlib(Module), setupStdio(Module)]);
    Module.removeRunDependency("pre-run");
  },
};

try {
  await createEmscriptenModule(emscriptenSettings);
} catch (e) {
  // Show JavaScript exception and traceback
  console.warn(e);
  // Show Python exception and traceback
  Module.__Py_DumpTraceback(2, Module._PyGILState_GetThisThreadState());
  process.exit(1);
}
