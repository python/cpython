import EmscriptenModule from "./python.mjs";
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';

if (process?.versions?.node) {
  const nodeVersion = Number(process.versions.node.split(".", 1)[0]);
  if (nodeVersion < 18) {
      process.stderr.write(
          `Node version must be >= 18, got version ${process.version}\n`,
      );
      process.exit(1);
  }
}

const settings = {
  preRun(Module) {
    const __dirname = dirname(fileURLToPath(import.meta.url));
    Module.FS.mkdirTree("/lib/");
    Module.FS.mount(Module.FS.filesystems.NODEFS, { root: __dirname + "/lib/" }, "/lib/");
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
