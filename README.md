# tweeta-cli

`tweeta` is a C/libcurl command-line client for Tweetapus, designed for AI agents and automation.

It exposes:

- Named subcommands for the full Tweetapus API, including admin-only endpoints.
- A universal request escape hatch for debugging or newly-added upstream endpoints.
- Tweetapus-compatible request headers required by the upstream API.

Build from source:

```sh
make
```

Install from npm:

```sh
npm install -g tweetapus
```

The npm package is published as `tweetapus` and contains prebuilt static `tweeta` binaries for all supported platforms, so installation on the end user's machine does not compile anything and does not fetch additional artifacts.
This makes the npm install much larger than building or installing with `make`: the binaries are statically linked except for libc on macOS, which cannot be statically linked and therefore is not, and the package includes all four platform binaries regardless of the platform installing it. As of this writing, a locally built dynamically linked AMD64 Linux binary is 148KB on my system because it only includes one binary for that machine; the npm install is nearly 61MB.

The package is published from CI after a successful default-branch release.

When the package is published, it is intended to be assembled before publish by downloading release assets from `https://github.com/tweetapus/tweeta-cli/releases/latest`. It expects assets named:

- `tweeta-linux-x64`
- `tweeta-linux-arm64`
- `tweeta-darwin-x64`
- `tweeta-darwin-arm64`

For packaging, you can override the release API URL with `TWEETA_INSTALL_RELEASE_URL`, or inject the full release payload with `TWEETA_INSTALL_RELEASE_JSON`.
If `npm-binaries/` is already populated, `prepack` will reuse those files; set `TWEETA_PREPACK_REFRESH=1` to force a fresh download during publish.

Publishing:

GitHub Actions is intended to publish a GitHub release on every branch push. The generated release version is numeric-only and uses the form `0.YYYYMMDD.RUN_NUMBER`. Pushes outside the default branch are marked as prereleases so the default-branch release remains the one exposed by `releases/latest`.

Configure:

```sh
./tweeta config set-base https://example.com
./tweeta auth login alice 'password'
```

If installed from npm, use `tweeta` instead of `./tweeta`.

Named API access:

```sh
./tweeta routes
./tweeta auth me
./tweeta tweets create --content 'hello'
./tweeta admin verify USER_ID --verified true
./tweeta admin update-user USER_ID --character_limit 10000
./tweeta admin delete-post POST_ID
```

Examples:

```sh
./tweeta me
./tweeta timeline home --limit 20
./tweeta tweet create --content 'hello from an agent'
./tweeta tweets create --content 'hello from the full command table'
./tweeta tweet like POST_ID
./tweeta profile get alice
./tweeta profile follow alice
./tweeta upload media ./image.png
./tweeta upload get --id POST_ID > attachment.bin
./tweeta upload get --id POST_ID --all
./tweeta upload get --id POST_ID --really-all > first-attachment.bin
./tweeta admin users --search alice
./tweeta admin user USER_ID
./tweeta admin suspend USER_ID --reason spam --action suspend --duration 60
./tweeta admin unsuspend USER_ID
./tweeta admin stats
```

Command format:

```sh
./tweeta GROUP ACTION [POSITIONAL_ARGS...] [--field value...] [--file path]
```

GET commands turn `--field value` pairs into query parameters. Mutating commands turn them into JSON bodies automatically, with `true`, `false`, `null`, numbers, arrays, and objects preserved as typed JSON values. Multipart endpoints use `--file`.

Configuration defaults to `$XDG_CONFIG_HOME/tweeta-cli/config` or `~/.config/tweeta-cli/config`.
You can override it with environment variables:

- `TWEETA_BASE_URL`
- `TWEETA_TOKEN`
- `TWEETA_CONFIG`

Run `./tweeta help` for the complete command list.

See [ROUTES.md](ROUTES.md) for detailed CLI usage for every named route.
