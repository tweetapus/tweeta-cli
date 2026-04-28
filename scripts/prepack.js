#!/usr/bin/env node

const fs = require("node:fs");
const https = require("node:https");
const path = require("node:path");

const rootDir = path.resolve(__dirname, "..");
const binariesDir = path.join(rootDir, "npm-binaries");
const releaseUrl =
  process.env.TWEETA_INSTALL_RELEASE_URL ||
  "https://api.github.com/repos/tweetapus/tweeta-cli/releases/latest";
const forceRefresh = process.env.TWEETA_PREPACK_REFRESH === "1";
const supportedTargets = [
  ["linux", "x64"],
  ["linux", "arm64"],
  ["darwin", "x64"],
  ["darwin", "arm64"]
];

function assetName(platform, arch) {
  return `tweeta-${platform}-${arch}`;
}

function httpGet(url, headers = {}, redirectCount = 0) {
  return new Promise((resolve, reject) => {
    const request = https.get(
      url,
      {
        headers: {
          "user-agent": "tweeta-cli-npm-prepack",
          accept: "application/vnd.github+json",
          ...headers
        }
      },
      (response) => {
        const { statusCode = 0, headers: responseHeaders } = response;

        if (statusCode >= 300 && statusCode < 400 && responseHeaders.location) {
          response.resume();
          if (redirectCount >= 5) {
            reject(new Error(`too many redirects while fetching ${url}`));
            return;
          }
          resolve(httpGet(responseHeaders.location, headers, redirectCount + 1));
          return;
        }

        if (statusCode < 200 || statusCode >= 300) {
          let body = "";
          response.setEncoding("utf8");
          response.on("data", (chunk) => {
            body += chunk;
          });
          response.on("end", () => {
            reject(new Error(`request failed for ${url}: HTTP ${statusCode}${body ? ` ${body}` : ""}`));
          });
          return;
        }

        const chunks = [];
        response.on("data", (chunk) => {
          chunks.push(chunk);
        });
        response.on("end", () => {
          resolve(Buffer.concat(chunks));
        });
      }
    );

    request.on("error", reject);
  });
}

function readFileUrl(url) {
  const filePath = new URL(url);
  return fs.readFileSync(filePath);
}

async function fetchBuffer(url, headers = {}) {
  if (url.startsWith("file:")) {
    return readFileUrl(url);
  }
  if (url.startsWith("https://")) {
    return httpGet(url, headers);
  }
  throw new Error(`unsupported asset URL: ${url}`);
}

async function readRelease() {
  if (process.env.TWEETA_INSTALL_RELEASE_JSON) {
    return JSON.parse(process.env.TWEETA_INSTALL_RELEASE_JSON);
  }
  const body = await fetchBuffer(releaseUrl);
  return JSON.parse(body.toString("utf8"));
}

function findAssetUrl(release, name) {
  if (!release || !Array.isArray(release.assets)) {
    throw new Error("release payload did not contain an assets array");
  }

  const asset = release.assets.find((item) => item && item.name === name);
  if (!asset || !asset.browser_download_url) {
    const available = release.assets
      .map((item) => item && item.name)
      .filter(Boolean)
      .join(", ");
    throw new Error(`release asset ${name} was not found${available ? `; available assets: ${available}` : ""}`);
  }

  return asset.browser_download_url;
}

function outputPath(name) {
  return path.join(binariesDir, name);
}

function haveAllAssets() {
  return supportedTargets.every(([platform, arch]) => fs.existsSync(outputPath(assetName(platform, arch))));
}

async function downloadAssets() {
  if (!forceRefresh && haveAllAssets()) {
    return;
  }

  const release = await readRelease();
  fs.mkdirSync(binariesDir, { recursive: true });

  for (const [platform, arch] of supportedTargets) {
    const name = assetName(platform, arch);
    const downloadUrl = findAssetUrl(release, name);
    const binary = await fetchBuffer(downloadUrl, {
      accept: "application/octet-stream"
    });
    const destination = outputPath(name);
    fs.writeFileSync(destination, binary, { mode: 0o755 });
    fs.chmodSync(destination, 0o755);
  }
}

module.exports = {
  assetName,
  findAssetUrl,
  supportedTargets
};

if (require.main === module) {
  downloadAssets().catch((error) => {
    console.error(`failed to bundle tweeta release binaries: ${error.message}`);
    process.exit(1);
  });
}
