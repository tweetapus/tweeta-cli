---
name: tweeta-cli
description: Use when asked to operate the installed tweeta CLI to talk to a Tweetapus server, including configuring auth or base URL, discovering named routes, sending requests, uploading or downloading attachments, or troubleshooting CLI request shapes.
---

# Tweeta CLI

Use this skill when the user wants to interact with Tweetapus through the installed `tweeta` command rather than by crafting raw HTTP by hand.

## Workflow

1. Confirm the CLI exists with `command -v tweeta` or `tweeta help`.
2. Inspect current client configuration with `tweeta config show`.
3. Prefer named routes over raw requests. Use `tweeta routes` for the compiled route table.
4. Use `tweeta request METHOD PATH ...` or `tweeta get|post|patch|put|delete ...` only when a named route is missing or the user explicitly wants raw request control.
5. If the environment should avoid writing credentials to disk, prefer temporary `TWEETA_*` environment variables over `config set-token`.

## Configuration And Auth

- Persistent config lives at `$XDG_CONFIG_HOME/tweeta-cli/config` or `~/.config/tweeta-cli/config`.
- One-off sessions can use `TWEETA_BASE_URL`, `TWEETA_TOKEN`, and `TWEETA_CONFIG`. Prefer environment overrides when you should avoid writing credentials to disk.
- Common setup:

```sh
tweeta config set-base https://example.com
tweeta auth login USER PASS
tweeta auth me
```

- `tweeta auth login` and `tweeta auth register` try to extract a token from the response and save it to config automatically.

## Command Semantics

- General form: `tweeta GROUP ACTION [POSITIONAL_ARGS...] [--field value ...] [--file PATH]`
- Positional arguments fill route placeholders like `:id` or `:username` from left to right.
- Path arguments are substituted verbatim. If a value needs URL encoding, encode it yourself before passing it.
- Query routes turn `--field value` pairs into URL query params. A bare flag becomes `name=true`.
- Body routes turn `--field value` pairs into a JSON object. The CLI preserves `true`, `false`, `null`, numbers, arrays, and objects as typed JSON values.
- Multipart routes use `--file PATH`.
- The legacy `tweet` alias is convenient, but `tweet create` stringifies values more aggressively than `tweets create`. Prefer `tweets create` when booleans, numbers, arrays, or objects matter.

## Common Commands

- Current user: `tweeta me`
- Home timeline: `tweeta timeline home --limit 20`
- Create a post: `tweeta tweets create --content 'hello'`
- Update a post: `tweeta tweets update POST_ID --content 'edited'`
- Profile lookup: `tweeta profile get alice`
- Follow a user: `tweeta profile follow alice`
- Upload media: `tweeta upload media ./image.png`
- Download attachments by post id: `tweeta upload get --id POST_ID --all`
- Admin examples: `tweeta admin users --search alice`, `tweeta admin stats`

## Route Discovery

- For the exact compiled commands, run `tweeta routes`.
- For quick route-finding patterns and payload-shape reminders, read [references/route-discovery.md](references/route-discovery.md).
- If local CLI output is not enough and the environment permits network access, the upstream route reference is available at `https://raw.githubusercontent.com/tweetapus/tweeta-cli/refs/heads/master/ROUTES.md`. Only fetch it if needed and with the user's approval.

## Validation

- Sanity-check the binary with `tweeta help`.
- Verify config with `tweeta config show`.
- After auth is configured, use `tweeta auth me` as the quickest connectivity check.
- When a request fails, confirm whether you chose the correct named route and payload shape before assuming a server bug.
