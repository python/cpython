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
      .filter((dir) => !["dev", "lib", "proc"].includes(dir)).map(dir => "/" + dir);
}

function mountDirectories(Module) {
  for (const dir of rootDirsToMount(Module)) {
    Module.FS.mkdirTree(dir);
    Module.FS.mount(Module.FS.filesystems.NODEFS, { root: dir }, dir);
  }
}

const settings = {
  preRun(Module) {
    Module.FS.mkdirTree("/home/");
    mountDirectories(Module);
    Module.FS.chdir(process.cwd());
  },
  // The first three arguments are: "node", path to this file, path to
  // python.sh. After that come the arguments the user passed to python.sh.
  arguments: process.argv.slice(3),
  // Ensure that sys.executable, sys._base_executable, etc point to python.sh
  // not to this file. To properly handle symlinks, python.sh needs to compute
  // its own path.
  thisProgram: process.argv[2],
};

await EmscriptenModule(settings);
