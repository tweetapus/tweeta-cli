#!/usr/bin/env node

const { spawn } = require("node:child_process");
const fs = require("node:fs");
const path = require("node:path");

function mapPlatform(platform) {
  if (platform === "linux" || platform === "darwin") return platform;
  return null;
}

function mapArch(arch) {
  if (arch === "x64" || arch === "arm64") return arch;
  return null;
}

function bundledBinaryPath(platform = process.platform, arch = process.arch) {
  const mappedPlatform = mapPlatform(platform);
  const mappedArch = mapArch(arch);

  if (!mappedPlatform || !mappedArch) {
    return null;
  }

  return path.resolve(__dirname, "..", "npm-binaries", `tweeta-${mappedPlatform}-${mappedArch}`);
}

const binaryPath = bundledBinaryPath();

if (!binaryPath) {
  console.error(`tweeta-cli does not support ${process.platform}/${process.arch}.`);
  process.exit(1);
}

if (!fs.existsSync(binaryPath)) {
  console.error(`bundled tweeta binary not found at ${binaryPath}.`);
  process.exit(1);
}

const child = spawn(binaryPath, process.argv.slice(2), {
  stdio: "inherit"
});

const forwardSignal = (signal) => {
  if (child.exitCode === null && child.signalCode === null) {
    child.kill(signal);
  }
};

process.on("SIGINT", () => forwardSignal("SIGINT"));
process.on("SIGTERM", () => forwardSignal("SIGTERM"));

child.on("error", (error) => {
  console.error(`failed to launch tweeta: ${error.message}`);
  process.exit(1);
});

child.on("exit", (code, signal) => {
  if (signal) {
    process.kill(process.pid, signal);
    return;
  }
  process.exit(code ?? 1);
});
