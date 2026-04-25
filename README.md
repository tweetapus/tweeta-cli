# tweeta-cli

`tweeta` is a C/libcurl command-line client for Tweetapus, designed for AI agents and automation.

It exposes:

- Named subcommands for the full Tweetapus API, including admin-only endpoints.
- A universal request escape hatch for debugging or newly-added upstream endpoints.
- Tweetapus-compatible request headers required by the upstream API.

Build:

```sh
make
```

Configure:

```sh
./tweeta config set-base https://example.com
./tweeta auth login alice 'password'
```

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
