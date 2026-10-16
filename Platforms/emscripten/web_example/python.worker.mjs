import createEmscriptenModule from "./python.mjs";

class StdinBuffer {
  constructor() {
    this.sab = new SharedArrayBuffer(128 * Int32Array.BYTES_PER_ELEMENT);
    this.buffer = new Int32Array(this.sab);
    this.readIndex = 1;
    this.numberOfCharacters = 0;
    this.sentNull = true;
  }

  prompt() {
    this.readIndex = 1;
    Atomics.store(this.buffer, 0, -1);
    postMessage({
      type: "stdin",
      buffer: this.sab,
    });
    Atomics.wait(this.buffer, 0, -1);
    this.numberOfCharacters = this.buffer[0];
  }

  stdin = () => {
    while (this.numberOfCharacters + 1 === this.readIndex) {
      if (!this.sentNull) {
        // Must return null once to indicate we're done for now.
        this.sentNull = true;
        return null;
      }
      this.sentNull = false;
      // Prompt will reset this.readIndex to 1
      this.prompt();
    }
    const char = this.buffer[this.readIndex];
    this.readIndex += 1;
    return char;
  };
}

const stdout = (charCode) => {
  if (charCode) {
    postMessage({
      type: "stdout",
      stdout: charCode,
    });
  } else {
    console.log(typeof charCode, charCode);
  }
};

const stderr = (charCode) => {
  if (charCode) {
    postMessage({
      type: "stderr",
      stderr: charCode,
    });
  } else {
    console.log(typeof charCode, charCode);
  }
};

const stdinBuffer = new StdinBuffer();

const emscriptenSettings = {
  noInitialRun: true,
  stdin: stdinBuffer.stdin,
  stdout: stdout,
  stderr: stderr,
  onRuntimeInitialized: () => {
    postMessage({ type: "ready", stdinBuffer: stdinBuffer.sab });
  },
  async preRun(Module) {
    const versionInt = Module.HEAPU32[Module._Py_Version >>> 2];
    const major = (versionInt >>> 24) & 0xff;
    const minor = (versionInt >>> 16) & 0xff;
    // Prevent complaints about not finding exec-prefix by making a lib-dynload directory
    Module.FS.mkdirTree(`/lib/python${major}.${minor}/lib-dynload/`);
    Module.addRunDependency("install-stdlib");
    const resp = await fetch(`python${major}.${minor}.zip`);
    const stdlibBuffer = await resp.arrayBuffer();
    Module.FS.writeFile(
      `/lib/python${major}${minor}.zip`,
      new Uint8Array(stdlibBuffer),
      { canOwn: true },
    );
    Module.removeRunDependency("install-stdlib");
  },
};

const modulePromise = createEmscriptenModule(emscriptenSettings);

onmessage = async (event) => {
  if (event.data.type === "run") {
    const Module = await modulePromise;
    if (event.data.files) {
      for (const [filename, contents] of Object.entries(event.data.files)) {
        Module.FS.writeFile(filename, contents);
      }
    }
    const ret = Module.callMain(event.data.args);
    postMessage({
      type: "finished",
      returnCode: ret,
    });
  }
};
