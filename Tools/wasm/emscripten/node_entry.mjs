import EmscriptenModule from "./python.mjs";
import fs from "node:fs";

if (process?.versions?.node) {
  const nodeVersion = Number(process.versions.node.split(".", 1)[0]);
  if (nodeVersion < 18) {
    process.stderr.write(
      `Node version must be >= 18, got version ${process.version}\n`,
    );
    process.exit(1);
  }
}

function rootDirsToMount(Module) {
  return fs
    .readdirSync("/")
    .filter((dir) => !["dev", "lib", "proc"].includes(dir))
    .map((dir) => "/" + dir);
}

function mountDirectories(Module) {
  for (const dir of rootDirsToMount(Module)) {
    Module.FS.mkdirTree(dir);
    Module.FS.mount(Module.FS.filesystems.NODEFS, { root: dir }, dir);
  }
}

const thisProgram = "--this-program=";
const thisProgramIndex = process.argv.findIndex((x) =>
  x.startsWith(thisProgram),
);

const settings = {
  preRun(Module) {
    // Globally expose API object so we can access it if we raise on startup.
    globalThis.Module = Module;
    mountDirectories(Module);
    Module.FS.chdir(process.cwd());
    Object.assign(Module.ENV, process.env);
    delete Module.ENV.PATH;
  },
  // Ensure that sys.executable, sys._base_executable, etc point to python.sh
  // not to this file. To properly handle symlinks, python.sh needs to compute
  // its own path.
  thisProgram: process.argv[thisProgramIndex].slice(thisProgram.length),
  // After python.sh come the arguments thatthe user passed to python.sh.
  arguments: process.argv.slice(thisProgramIndex + 1),
};

try {
  await EmscriptenModule(settings);
} catch(e) {
  // Show JavaScript exception and traceback
  console.warn(e);
  // Show Python exception and traceback
  Module.__Py_DumpTraceback(2, Module._PyGILState_GetThisThreadState());
  process.exit(1);
}
