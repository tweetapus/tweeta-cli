# Route Discovery

Use this file when the installed CLI is available but the source repository and its docs are not.

## Quick Checks

- List all compiled commands:

```sh
tweeta routes
```

- Show top-level help:

```sh
tweeta help
```

- Inspect the current client configuration:

```sh
tweeta config show
```

## Heuristics

- Prefer named routes over `request` unless the endpoint is missing from `tweeta routes`.
- Prefer `tweets ...` over the legacy `tweet ...` alias when typed JSON matters.
- Start with `tweeta help` and `tweeta routes`; they are the most portable sources in installed environments.
- If those are not sufficient and the user allows network access, fetch `https://raw.githubusercontent.com/tweetapus/tweeta-cli/refs/heads/master/ROUTES.md` for the full upstream route guide.
- If you need to probe an undocumented endpoint, fall back to `tweeta request`.

## Common Patterns

- Query route:

```sh
tweeta timeline home --limit 20 --cursor abc123 --include_replies
```

- JSON body route:

```sh
tweeta tweets create --content 'hello' --sensitive false --poll '[1,2]'
```

- Multipart route:

```sh
tweeta profile set-avatar alice --file ./avatar.png
```

- Raw request fallback:

```sh
tweeta request POST /api/some/new/endpoint --json '{"enabled":true}'
```
