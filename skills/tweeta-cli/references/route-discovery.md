# Route Discovery

Use this file when the installed CLI is available but the source repository and full route guide are not.

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

## Route Selection

- Prefer named routes over `request` unless the endpoint is missing from `tweeta routes` or exact raw HTTP control matters.
- `tweeta routes` is the local source of truth for the installed binary. It may differ from repository docs if the installed binary is older or newer.
- Path placeholders are filled by positional arguments from left to right. They are substituted verbatim; URL-encode path values yourself.
- If positional arguments are missing, the CLI fails locally and does not send a request.
- Repeated flags are not deduplicated. Query routes emit repeated query parameters; body routes emit repeated JSON keys.
- Reserved client flags such as `--all`, `--short`, and `--file` are consumed by the client rather than transmitted as normal fields.

## Heuristics

- Prefer `tweets ...` over the legacy `tweet ...` alias when typed JSON matters.
- Start with `tweeta help` and `tweeta routes`; they are the most portable sources in installed environments.
- If those are not sufficient and the user allows network access, fetch `https://raw.githubusercontent.com/tweetapus/tweeta-cli/refs/heads/master/ROUTES.md` for the full upstream route guide.
- If you need to probe an undocumented endpoint, fall back to `tweeta request`.

## Payload Shapes

- Query route:

```sh
tweeta timeline home --limit 20 --cursor abc123 --include_replies
```

- JSON body route:

```sh
tweeta tweets create --content 'hello' --sensitive false --poll '[1,2]'
```

- JSON literals must survive shell parsing. Quote arrays and objects:

```sh
tweeta lists update LIST_ID --members '["alice","bob"]' --settings '{"private":true}'
```

- Multipart route:

```sh
tweeta profile set-avatar alice --file ./avatar.png
```

## Aliases And Helpers

- `tweeta me` is a shortcut for `tweeta auth me`.
- `tweeta timeline [--field value ...]` defaults to the home timeline when no action name is supplied.
- `tweeta tweet create|get|like|retweet|delete|reactions ...` is a legacy shortcut family. Prefer `tweets ...` when JSON type preservation matters.
- `tweeta upload media FILE` is a shortcut for uploading a file to `/api/upload`.
- `tweeta upload get --id POST_ID [--all] [--really-all]` fetches a post, extracts attachment references, then streams or downloads attachment files. It is different from the named route `tweeta upload get FILENAME`, which fetches a known upload filename/path.

## Raw Request Fallback

- Full request form:

```sh
tweeta request POST /api/some/new/endpoint --json '{"enabled":true}'
```

- Text body:

```sh
tweeta request POST /api/some/new/endpoint --data 'plain text'
```

- File body:

```sh
tweeta request PUT /api/some/new/endpoint --data-file ./payload.json
```

- Multipart upload:

```sh
tweeta request POST /api/some/upload --upload file ./image.png
```

- Raw request fallback:

```sh
tweeta post /api/some/new/endpoint --json '{"enabled":true}'
```
