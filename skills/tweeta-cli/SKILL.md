---
name: tweeta-cli
description: Use when asked to operate the installed tweeta CLI to talk to a Tweetapus server, including configuring auth or base URL, discovering named routes, sending requests, uploading or downloading attachments, or troubleshooting CLI request shapes.
---

# Tweeta CLI

Use this skill when the user wants to interact with Tweetapus through the installed `tweeta` command instead of hand-crafting HTTP.

## Workflow

1. Confirm the CLI exists with `command -v tweeta` or `tweeta help`.
2. Inspect current client configuration with `tweeta config show`; do not assume the default server is intended.
3. Prefer named routes. Run `tweeta routes` when the exact group/action or payload shape is unclear.
4. Use raw `tweeta request METHOD PATH ...` or `tweeta get|post|patch|put|delete ...` only when a named route is missing or the user explicitly wants raw request control.
5. If the task involves credentials or a temporary server, prefer one-off `TWEETA_*` environment variables over writing persistent config.
6. After failures, first re-check base URL, auth, route name, positional arguments, and payload shape before assuming a server bug.

## Configuration And Auth

- Persistent config lives at `$XDG_CONFIG_HOME/tweeta-cli/config` or `~/.config/tweeta-cli/config`.
- One-off sessions can use `TWEETA_BASE_URL`, `TWEETA_TOKEN`, and `TWEETA_CONFIG`.
- If no base URL is configured, the compiled default is `https://tweeta.tiago.zip`.
- Common setup:

```sh
tweeta config set-base https://example.com
tweeta auth login USER PASS
tweeta auth me
```

- `tweeta auth login` and `tweeta auth register` try to extract a token from the response and save it to config automatically.

## Command Semantics

- General form: `tweeta GROUP ACTION [POSITIONAL_ARGS...] [--field value ...] [--file PATH]`
- Positional arguments fill route placeholders like `:id` or `:username` from left to right. Missing positional arguments fail locally and do not send a request.
- Path arguments are substituted verbatim; URL-encode path values yourself when needed.
- Query routes turn `--field value` pairs into URL query params. A bare flag becomes `name=true`.
- Body routes turn `--field value` pairs into a JSON object. The CLI preserves `true`, `false`, `null`, numbers, arrays, and objects as typed JSON values.
- Quote shell-sensitive JSON literals, for example `--members '["a","b"]'` or `--settings '{"dark":true}'`.
- Multipart named routes use `--file PATH`; some upload helpers accept the file as a positional argument.
- Reserved flags: `--all`, `--short`, and `--file` are handled by the client and are not transmitted as ordinary query/body fields.
- The legacy `tweet` alias is convenient, but `tweet create` stringifies values more aggressively than `tweets create`. Prefer `tweets create` when booleans, numbers, arrays, or objects matter.

## Raw Request Fallback

- Full form: `tweeta request METHOD PATH [--json JSON | --data TEXT | --data-file FILE | --upload FIELD FILE]`
- Shorthand form: `tweeta get|post|patch|put|delete PATH [--json JSON]`
- `get` and `delete` shorthands send no body. `post`, `patch`, and `put` default to `{}` when `--json` is omitted.
- Use raw requests for newly added upstream endpoints, debugging exact HTTP behavior, or endpoints intentionally not represented in the named route table.

## Common Commands

- Current user: `tweeta me`
- Home timeline: `tweeta timeline home --limit 20`
- Create a post: `tweeta tweets create --content 'hello'`
- Update a post: `tweeta tweets update POST_ID --content 'edited'`
- Profile lookup: `tweeta profile get alice`
- Follow a user: `tweeta profile follow alice`
- Upload media: `tweeta upload media ./image.png`
- Download attachments by post id: `tweeta upload get --id POST_ID --all`
- Fetch a known upload path: `tweeta upload get FILENAME`
- Admin examples: `tweeta admin users --search alice`, `tweeta admin stats`

## Route Discovery

- For the exact compiled commands, run `tweeta routes`.
- For route-finding patterns, payload-shape details, aliases, and upload/download distinctions, read [references/route-discovery.md](references/route-discovery.md).
- If local CLI output is not enough and the environment permits network access, the upstream route reference is available at `https://raw.githubusercontent.com/tweetapus/tweeta-cli/refs/heads/master/ROUTES.md`. Only fetch it if needed and with the user's approval.

## Validation

- Sanity-check the binary with `tweeta help`.
- Verify config with `tweeta config show`.
- After auth is configured, use `tweeta auth me` as the quickest connectivity check.
- For route-selection problems, compare `tweeta routes` with [references/route-discovery.md](references/route-discovery.md) before falling back to raw HTTP.
