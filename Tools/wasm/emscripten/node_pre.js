// If process is undefined, we're not running in the node runtime let it go I
// guess?
if (typeof process !== "undefined") {
    const nodeVersion = Number(process.versions.node.split('.',1)[0]);
    if (nodeVersion < 18) {
        process.stderr.write(`Node version must be >= 18, got version ${process.version}\n`);
        process.exit(1);
    }
}
