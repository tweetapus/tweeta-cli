# ROUTES

This file documents every named route exposed by `tweeta` as a CLI command. It is intentionally CLI-focused: it explains how to invoke each route, how positional arguments are substituted, and how the client shapes your flags into a request. It does **not** attempt to describe the upstream API schema, validation rules, or response payload contracts.

The source of truth for the route table is [`src/routes.c`](src/routes.c). At the time of writing, the CLI exposes **309 named routes** across **29 route groups**.

## Before You Start

- Build the CLI with `make`.
- Configure a base URL with `./tweeta config set-base URL`. If you do nothing, the compiled default is `https://tweeta.tiago.zip`.
- Authenticate with either `./tweeta auth login USER PASS`, `./tweeta auth register USER PASS`, or `./tweeta config set-token TOKEN`.
- You can inspect the configured state with `./tweeta config show`.
- Environment overrides: `TWEETA_BASE_URL`, `TWEETA_TOKEN`, and `TWEETA_CONFIG`.

## How Named Routes Work

- General syntax: `./tweeta GROUP ACTION [POSITIONAL_ARGS...] [--field value ...]`.
- The CLI matches `GROUP` and `ACTION` against the compiled route table. `./tweeta routes` prints that table exactly as built.
- Path placeholders like `:id` or `:username` are filled from left to right using positional arguments after `ACTION`. The substitution is **verbatim**. The client does not URL-encode positional path arguments for you.
- If you do not supply enough positional arguments, the CLI fails locally with `tweeta: GROUP ACTION requires N positional argument(s)` and does not send a request.
- The CLI does not validate whether a field name is supported by the upstream server. For query/body routes, it only converts your `--field value` pairs into the transport shape expected by that route.
- Reserved client-side flags: `--all` is consumed for expanded output formatting and is never transmitted as a server field. `--short` is also stripped before query/body generation. `--file` is stripped from JSON/query generation and is only meaningful for file-upload routes.
- Repeated flags are emitted repeatedly. For query routes that means repeated query parameters. For body routes that means repeated JSON keys in the generated object string; the CLI does not deduplicate or normalize them.
- For body routes, quote JSON literals in the shell when needed, for example `--members '["a","b"]'` or `--settings '{"dark":true}'`, so the CLI can pass them through as raw JSON values.

## Request Shapes

### Query routes (`PAYLOAD_QUERY`)

After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.

Example: `./tweeta timeline home --limit 20 --cursor abc123 --include_replies` becomes `GET /api/timeline/?limit=20&cursor=abc123&include_replies=true`.

### JSON body routes (`PAYLOAD_BODY`)

After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.

Example: `./tweeta tweets update POST_ID --content 'edited' --sensitive false --poll '[1,2]'` becomes a JSON body roughly like `{"content":"edited","sensitive":false,"poll":[1,2]}`.

### Multipart upload routes (`PAYLOAD_FILE`)

This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.

Example: `./tweeta profile set-avatar USERNAME --file ./avatar.png` uploads `./avatar.png` as multipart field `avatar`.

### No-payload routes (`PAYLOAD_NONE`)

This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.

Example: `./tweeta tweets delete POST_ID` sends a bare `DELETE` to the resolved path.

## Convenience Aliases And Special Commands

These are not separate named routes, but they matter when you are working from the CLI instead of thinking in raw HTTP terms:

- `./tweeta me` is a shortcut for `./tweeta auth me`.
- `./tweeta timeline [--field value ...]` is a shortcut for `./tweeta timeline home ...` when you omit the action name entirely.
- `./tweeta tweet create|get|like|retweet|delete|reactions ...` is a legacy shortcut family for a subset of `tweets` routes. Be careful with `tweet create`: it turns every option value into a JSON string, unlike `tweets create`, which preserves booleans/numbers/raw JSON literals.
- `./tweeta profile get|follow|unfollow|followers|following USERNAME` has shortcut handling in `main.c`, but the named-route forms are the same commands and are preferred for consistency.
- `./tweeta upload media FILE` is a shortcut for the raw upload route.
- `./tweeta upload get --id POST_ID [--all] [--really-all]` is **not** the same as the named route `upload get FILENAME`. It first fetches the tweet JSON, extracts attachment references, and then streams or downloads the attachment(s). Use the named route when you already have the upload filename/path; use the helper when you only have a post ID.
- `./tweeta admin stats|users|user|suspend|unsuspend ...` also has shortcut handling. The named routes still exist and are documented below.
- `./tweeta request METHOD PATH ...` and the `get|post|patch|put|delete` shorthand commands bypass the named-route table entirely. They are useful when upstream adds an endpoint before this CLI gains a named route.

## Group Index

- [`core`](#core) - 5 route(s)
- [`auth`](#auth) - 18 route(s)
- [`tweets`](#tweets) - 17 route(s)
- [`timeline`](#timeline) - 5 route(s)
- [`profile`](#profile) - 41 route(s)
- [`bookmarks`](#bookmarks) - 3 route(s)
- [`blocking`](#blocking) - 7 route(s)
- [`muted`](#muted) - 7 route(s)
- [`communities`](#communities) - 31 route(s)
- [`delegates`](#delegates) - 10 route(s)
- [`dm`](#dm) - 22 route(s)
- [`notifications`](#notifications) - 4 route(s)
- [`push`](#push) - 4 route(s)
- [`lists`](#lists) - 14 route(s)
- [`articles`](#articles) - 3 route(s)
- [`search`](#search) - 2 route(s)
- [`public-tweets`](#public-tweets) - 1 route(s)
- [`scheduled`](#scheduled) - 3 route(s)
- [`reports`](#reports) - 1 route(s)
- [`translate`](#translate) - 1 route(s)
- [`trends`](#trends) - 1 route(s)
- [`tenor`](#tenor) - 1 route(s)
- [`unsplash`](#unsplash) - 1 route(s)
- [`sse`](#sse) - 1 route(s)
- [`upload`](#upload) - 2 route(s)
- [`explore`](#explore) - 16 route(s)
- [`mpi`](#mpi) - 6 route(s)
- [`shop`](#shop) - 8 route(s)
- [`admin`](#admin) - 74 route(s)

## core

Miscellaneous platform routes that do not fit a larger feature group.

### `core emojis`

- Command: `./tweeta core emojis [--field value ...]`
- HTTP target: `GET /api/emojis`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta core emojis --field value --flag`

### `core transparency`

- Command: `./tweeta core transparency USER [--field value ...]`
- HTTP target: `GET /api/transparency/:user`
- Path arguments: 1 positional argument. `USER` fills `:user`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta core transparency USER --field value --flag`

### `core transparency-asn`

- Command: `./tweeta core transparency-asn USER [--field value ...]`
- HTTP target: `GET /api/transparency/:user/asn`
- Path arguments: 1 positional argument. `USER` fills `:user`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta core transparency-asn USER --field value --flag`

### `core owoembed`

- Command: `./tweeta core owoembed [--field value ...]`
- HTTP target: `GET /api/owoembed`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta core owoembed --field value --flag`

### `core ack-warning`

- Command: `./tweeta core ack-warning [--field value ...]`
- HTTP target: `POST /api/warning/acknowledge`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta core ack-warning --field value --count 3 --enabled true`

## auth

Authentication, session, passkey, and account-management routes.

### `auth cap-config`

- Command: `./tweeta auth cap-config [--field value ...]`
- HTTP target: `GET /api/auth/cap/config`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta auth cap-config --field value --flag`

### `auth cap-rate-limit-bypass`

- Command: `./tweeta auth cap-rate-limit-bypass [--field value ...]`
- HTTP target: `POST /api/auth/cap/rate-limit-bypass`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth cap-rate-limit-bypass --field value --count 3 --enabled true`

### `auth me`

- Command: `./tweeta auth me [--field value ...]`
- HTTP target: `GET /api/auth/me`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta auth me --field value --flag`
- Notes: Top-level alias: `./tweeta me` does the same request.

### `auth switch-to-delegate`

- Command: `./tweeta auth switch-to-delegate [--field value ...]`
- HTTP target: `POST /api/auth/switch-to-delegate`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth switch-to-delegate --field value --count 3 --enabled true`

### `auth switch-to-primary`

- Command: `./tweeta auth switch-to-primary [--field value ...]`
- HTTP target: `POST /api/auth/switch-to-primary`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth switch-to-primary --field value --count 3 --enabled true`

### `auth add-account`

- Command: `./tweeta auth add-account [--field value ...]`
- HTTP target: `POST /api/auth/add-account`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth add-account --field value --count 3 --enabled true`

### `auth username-availability`

- Command: `./tweeta auth username-availability [--field value ...]`
- HTTP target: `GET /api/auth/username-availability`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta auth username-availability --field value --flag`

### `auth generate-registration-options`

- Command: `./tweeta auth generate-registration-options [--field value ...]`
- HTTP target: `POST /api/auth/generate-registration-options`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth generate-registration-options --field value --count 3 --enabled true`

### `auth verify-registration`

- Command: `./tweeta auth verify-registration [--field value ...]`
- HTTP target: `POST /api/auth/verify-registration`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth verify-registration --field value --count 3 --enabled true`

### `auth generate-authentication-options`

- Command: `./tweeta auth generate-authentication-options [--field value ...]`
- HTTP target: `POST /api/auth/generate-authentication-options`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth generate-authentication-options --field value --count 3 --enabled true`

### `auth verify-authentication`

- Command: `./tweeta auth verify-authentication [--field value ...]`
- HTTP target: `POST /api/auth/verify-authentication`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth verify-authentication --field value --count 3 --enabled true`

### `auth passkeys`

- Command: `./tweeta auth passkeys [--field value ...]`
- HTTP target: `GET /api/auth/passkeys`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta auth passkeys --field value --flag`

### `auth rename-passkey`

- Command: `./tweeta auth rename-passkey CRED_ID [--field value ...]`
- HTTP target: `PUT /api/auth/passkeys/:credId/name`
- Path arguments: 1 positional argument. `CRED_ID` fills `:credId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth rename-passkey CRED_ID --field value --count 3 --enabled true`

### `auth delete-passkey`

- Command: `./tweeta auth delete-passkey CRED_ID`
- HTTP target: `DELETE /api/auth/passkeys/:credId`
- Path arguments: 1 positional argument. `CRED_ID` fills `:credId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta auth delete-passkey CRED_ID`

### `auth register-password`

- Command: `./tweeta auth register-password [--field value ...]`
- HTTP target: `POST /api/auth/register-with-password`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth register-password --field value --count 3 --enabled true`

### `auth basic-login`

- Command: `./tweeta auth basic-login [--field value ...]`
- HTTP target: `POST /api/auth/basic-login`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth basic-login --field value --count 3 --enabled true`

### `auth moderation-history`

- Command: `./tweeta auth moderation-history [--field value ...]`
- HTTP target: `GET /api/auth/moderation-history`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta auth moderation-history --field value --flag`

### `auth validate-accounts`

- Command: `./tweeta auth validate-accounts [--field value ...]`
- HTTP target: `POST /api/auth/validate-accounts`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta auth validate-accounts --field value --count 3 --enabled true`

## tweets

Tweet creation, retrieval, engagement, and moderation-adjacent tweet operations.

### `tweets create`

- Command: `./tweeta tweets create [--field value ...]`
- HTTP target: `POST /api/tweets/`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets create --field value --count 3 --enabled true`
- Notes: Legacy alias: `./tweeta tweet create ...` is also available, but that alias stringifies every option value instead of using the named route's JSON auto-typing. Prefer `./tweeta tweets create` when you want booleans, numbers, arrays, or objects to stay typed.

### `tweets react`

- Command: `./tweeta tweets react ID [--field value ...]`
- HTTP target: `POST /api/tweets/:id/reaction`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets react ID --field value --count 3 --enabled true`

### `tweets reactions`

- Command: `./tweeta tweets reactions ID [--field value ...]`
- HTTP target: `GET /api/tweets/:id/reactions`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets reactions ID --field value --flag`
- Notes: Legacy alias: `./tweeta tweet reactions ID` is equivalent for the common case.

### `tweets get`

- Command: `./tweeta tweets get ID [--field value ...]`
- HTTP target: `GET /api/tweets/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets get ID --field value --flag`
- Notes: Legacy alias: `./tweeta tweet get ID` is equivalent for the common case.

### `tweets like`

- Command: `./tweeta tweets like ID [--field value ...]`
- HTTP target: `POST /api/tweets/:id/like`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets like ID --field value --count 3 --enabled true`
- Notes: Legacy alias: `./tweeta tweet like ID` is equivalent for the common case.

### `tweets retweet`

- Command: `./tweeta tweets retweet ID [--field value ...]`
- HTTP target: `POST /api/tweets/:id/retweet`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets retweet ID --field value --count 3 --enabled true`
- Notes: Legacy alias: `./tweeta tweet retweet ID` is equivalent for the common case.

### `tweets poll-vote`

- Command: `./tweeta tweets poll-vote ID [--field value ...]`
- HTTP target: `POST /api/tweets/:id/poll/vote`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets poll-vote ID --field value --count 3 --enabled true`

### `tweets likes`

- Command: `./tweeta tweets likes ID [--field value ...]`
- HTTP target: `GET /api/tweets/:id/likes`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets likes ID --field value --flag`

### `tweets retweets`

- Command: `./tweeta tweets retweets ID [--field value ...]`
- HTTP target: `GET /api/tweets/:id/retweets`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets retweets ID --field value --flag`

### `tweets quotes`

- Command: `./tweeta tweets quotes ID [--field value ...]`
- HTTP target: `GET /api/tweets/:id/quotes`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets quotes ID --field value --flag`

### `tweets can-reply`

- Command: `./tweeta tweets can-reply ID [--field value ...]`
- HTTP target: `GET /api/tweets/can-reply/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets can-reply ID --field value --flag`

### `tweets bulk-delete`

- Command: `./tweeta tweets bulk-delete [--field value ...]`
- HTTP target: `POST /api/tweets/bulk-delete`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets bulk-delete --field value --count 3 --enabled true`

### `tweets delete`

- Command: `./tweeta tweets delete ID`
- HTTP target: `DELETE /api/tweets/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta tweets delete ID`
- Notes: Legacy alias: `./tweeta tweet delete ID` is equivalent for the common case.

### `tweets reply-restriction`

- Command: `./tweeta tweets reply-restriction ID [--field value ...]`
- HTTP target: `PATCH /api/tweets/:id/reply-restriction`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets reply-restriction ID --field value --count 3 --enabled true`

### `tweets update`

- Command: `./tweeta tweets update ID [--field value ...]`
- HTTP target: `PUT /api/tweets/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets update ID --field value --count 3 --enabled true`

### `tweets outline`

- Command: `./tweeta tweets outline ID [--field value ...]`
- HTTP target: `PATCH /api/tweets/:id/outline`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta tweets outline ID --field value --count 3 --enabled true`

### `tweets edit-history`

- Command: `./tweeta tweets edit-history ID [--field value ...]`
- HTTP target: `GET /api/tweets/:id/edit-history`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tweets edit-history ID --field value --flag`

## timeline

Home/following timeline reads and interest-management operations.

### `timeline home`

- Command: `./tweeta timeline home [--field value ...]`
- HTTP target: `GET /api/timeline/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta timeline home --field value --flag`
- Notes: Top-level alias: `./tweeta timeline [--field value ...]` maps to this route when you omit a named action.

### `timeline following`

- Command: `./tweeta timeline following [--field value ...]`
- HTTP target: `GET /api/timeline/following`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta timeline following --field value --flag`

### `timeline interests`

- Command: `./tweeta timeline interests [--field value ...]`
- HTTP target: `GET /api/timeline/for-you/interests`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta timeline interests --field value --flag`

### `timeline clear-interests`

- Command: `./tweeta timeline clear-interests`
- HTTP target: `DELETE /api/timeline/for-you/interests`
- Path arguments: None.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta timeline clear-interests`

### `timeline delete-interest`

- Command: `./tweeta timeline delete-interest TOPIC`
- HTTP target: `DELETE /api/timeline/for-you/interests/:topic`
- Path arguments: 1 positional argument. `TOPIC` fills `:topic`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta timeline delete-interest TOPIC`

## profile

Profile reads, writes, relationships, settings, and profile media uploads.

### `profile get`

- Command: `./tweeta profile get USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile get USERNAME --field value --flag`
- Notes: Legacy alias: `./tweeta profile get USERNAME` is handled both as a named route and by a dedicated shortcut in `main.c`; use the named form shown here for consistency.

### `profile replies`

- Command: `./tweeta profile replies USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/replies`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile replies USERNAME --field value --flag`

### `profile media`

- Command: `./tweeta profile media USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/media`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile media USERNAME --field value --flag`

### `profile posts`

- Command: `./tweeta profile posts USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/posts`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile posts USERNAME --field value --flag`

### `profile highlights`

- Command: `./tweeta profile highlights USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/highlights`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile highlights USERNAME --field value --flag`

### `profile highlight`

- Command: `./tweeta profile highlight USERNAME POST_ID [--field value ...]`
- HTTP target: `POST /api/profile/:username/highlights/:postId`
- Path arguments: 2 positional arguments, in order: `USERNAME` fills `:username`; `POST_ID` fills `:postId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile highlight USERNAME POST_ID --field value --count 3 --enabled true`

### `profile update`

- Command: `./tweeta profile update USERNAME [--field value ...]`
- HTTP target: `PUT /api/profile/:username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile update USERNAME --field value --count 3 --enabled true`

### `profile follow`

- Command: `./tweeta profile follow USERNAME [--field value ...]`
- HTTP target: `POST /api/profile/:username/follow`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile follow USERNAME --field value --count 3 --enabled true`
- Notes: Legacy alias: `./tweeta profile follow USERNAME` is also handled by a dedicated shortcut in `main.c`.

### `profile unfollow`

- Command: `./tweeta profile unfollow USERNAME`
- HTTP target: `DELETE /api/profile/:username/follow`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta profile unfollow USERNAME`
- Notes: Legacy alias: `./tweeta profile unfollow USERNAME` is also handled by a dedicated shortcut in `main.c`.

### `profile notify-tweets`

- Command: `./tweeta profile notify-tweets USERNAME [--field value ...]`
- HTTP target: `POST /api/profile/:username/notify-tweets`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile notify-tweets USERNAME --field value --count 3 --enabled true`

### `profile set-avatar`

- Command: `./tweeta profile set-avatar USERNAME [--file PATH] [PATH]`
- HTTP target: `POST /api/profile/:username/avatar`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Multipart upload route
- Multipart field name: `avatar`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta profile set-avatar USERNAME --file ./avatar.png`

### `profile delete-avatar`

- Command: `./tweeta profile delete-avatar USERNAME`
- HTTP target: `DELETE /api/profile/:username/avatar`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta profile delete-avatar USERNAME`

### `profile set-banner`

- Command: `./tweeta profile set-banner USERNAME [--file PATH] [PATH]`
- HTTP target: `POST /api/profile/:username/banner`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Multipart upload route
- Multipart field name: `banner`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta profile set-banner USERNAME --file ./banner.png`

### `profile delete-banner`

- Command: `./tweeta profile delete-banner USERNAME`
- HTTP target: `DELETE /api/profile/:username/banner`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta profile delete-banner USERNAME`

### `profile followers`

- Command: `./tweeta profile followers USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/followers`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile followers USERNAME --field value --flag`
- Notes: Legacy alias: `./tweeta profile followers USERNAME` is also handled by a dedicated shortcut in `main.c`.

### `profile following`

- Command: `./tweeta profile following USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/following`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile following USERNAME --field value --flag`
- Notes: Legacy alias: `./tweeta profile following USERNAME` is also handled by a dedicated shortcut in `main.c`.

### `profile mutuals`

- Command: `./tweeta profile mutuals USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/mutuals`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile mutuals USERNAME --field value --flag`

### `profile followers-you-know`

- Command: `./tweeta profile followers-you-know USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/followers-you-know`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile followers-you-know USERNAME --field value --flag`

### `profile change-username`

- Command: `./tweeta profile change-username USERNAME [--field value ...]`
- HTTP target: `PATCH /api/profile/:username/username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile change-username USERNAME --field value --count 3 --enabled true`

### `profile change-password`

- Command: `./tweeta profile change-password USERNAME [--field value ...]`
- HTTP target: `PATCH /api/profile/:username/password`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile change-password USERNAME --field value --count 3 --enabled true`

### `profile outlines`

- Command: `./tweeta profile outlines USERNAME [--field value ...]`
- HTTP target: `PATCH /api/profile/:username/outlines`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile outlines USERNAME --field value --count 3 --enabled true`

### `profile delete`

- Command: `./tweeta profile delete USERNAME [--field value ...]`
- HTTP target: `DELETE /api/profile/:username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile delete USERNAME --field value --count 3 --enabled true`

### `profile set-password`

- Command: `./tweeta profile set-password USERNAME [--field value ...]`
- HTTP target: `POST /api/profile/:username/password`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile set-password USERNAME --field value --count 3 --enabled true`

### `profile follow-requests`

- Command: `./tweeta profile follow-requests [--field value ...]`
- HTTP target: `GET /api/profile/follow-requests`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile follow-requests --field value --flag`

### `profile approve-follow-request`

- Command: `./tweeta profile approve-follow-request REQUEST_ID [--field value ...]`
- HTTP target: `POST /api/profile/follow-requests/:requestId/approve`
- Path arguments: 1 positional argument. `REQUEST_ID` fills `:requestId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile approve-follow-request REQUEST_ID --field value --count 3 --enabled true`

### `profile deny-follow-request`

- Command: `./tweeta profile deny-follow-request REQUEST_ID [--field value ...]`
- HTTP target: `POST /api/profile/follow-requests/:requestId/deny`
- Path arguments: 1 positional argument. `REQUEST_ID` fills `:requestId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile deny-follow-request REQUEST_ID --field value --count 3 --enabled true`

### `profile affiliate`

- Command: `./tweeta profile affiliate USERNAME [--field value ...]`
- HTTP target: `POST /api/profile/:username/affiliate`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile affiliate USERNAME --field value --count 3 --enabled true`

### `profile affiliate-requests`

- Command: `./tweeta profile affiliate-requests [--field value ...]`
- HTTP target: `GET /api/profile/affiliate-requests`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile affiliate-requests --field value --flag`

### `profile approve-affiliate-request`

- Command: `./tweeta profile approve-affiliate-request REQUEST_ID [--field value ...]`
- HTTP target: `POST /api/profile/affiliate-requests/:requestId/approve`
- Path arguments: 1 positional argument. `REQUEST_ID` fills `:requestId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile approve-affiliate-request REQUEST_ID --field value --count 3 --enabled true`

### `profile deny-affiliate-request`

- Command: `./tweeta profile deny-affiliate-request REQUEST_ID [--field value ...]`
- HTTP target: `POST /api/profile/affiliate-requests/:requestId/deny`
- Path arguments: 1 positional argument. `REQUEST_ID` fills `:requestId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile deny-affiliate-request REQUEST_ID --field value --count 3 --enabled true`

### `profile affiliates`

- Command: `./tweeta profile affiliates USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/affiliates`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile affiliates USERNAME --field value --flag`

### `profile remove-affiliate`

- Command: `./tweeta profile remove-affiliate`
- HTTP target: `DELETE /api/profile/remove-affiliate`
- Path arguments: None.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta profile remove-affiliate`

### `profile pin`

- Command: `./tweeta profile pin TWEET_ID [--field value ...]`
- HTTP target: `POST /api/profile/pin/:tweetId`
- Path arguments: 1 positional argument. `TWEET_ID` fills `:tweetId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile pin TWEET_ID --field value --count 3 --enabled true`

### `profile unpin`

- Command: `./tweeta profile unpin TWEET_ID`
- HTTP target: `DELETE /api/profile/pin/:tweetId`
- Path arguments: 1 positional argument. `TWEET_ID` fills `:tweetId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta profile unpin TWEET_ID`

### `profile pin-for-user`

- Command: `./tweeta profile pin-for-user USERNAME TWEET_ID [--field value ...]`
- HTTP target: `POST /api/profile/:username/pin/:tweetId`
- Path arguments: 2 positional arguments, in order: `USERNAME` fills `:username`; `TWEET_ID` fills `:tweetId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile pin-for-user USERNAME TWEET_ID --field value --count 3 --enabled true`

### `profile unpin-for-user`

- Command: `./tweeta profile unpin-for-user USERNAME TWEET_ID`
- HTTP target: `DELETE /api/profile/:username/pin/:tweetId`
- Path arguments: 2 positional arguments, in order: `USERNAME` fills `:username`; `TWEET_ID` fills `:tweetId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta profile unpin-for-user USERNAME TWEET_ID`

### `profile private`

- Command: `./tweeta profile private [--field value ...]`
- HTTP target: `POST /api/profile/settings/private`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile private --field value --count 3 --enabled true`

### `profile community-tag`

- Command: `./tweeta profile community-tag [--field value ...]`
- HTTP target: `POST /api/profile/settings/community-tag`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile community-tag --field value --count 3 --enabled true`

### `profile transparency-location`

- Command: `./tweeta profile transparency-location [--field value ...]`
- HTTP target: `POST /api/profile/settings/transparency-location`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta profile transparency-location --field value --count 3 --enabled true`

### `profile algorithm-stats`

- Command: `./tweeta profile algorithm-stats USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/algorithm-stats`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile algorithm-stats USERNAME --field value --flag`

### `profile spam-score`

- Command: `./tweeta profile spam-score USERNAME [--field value ...]`
- HTTP target: `GET /api/profile/:username/spam-score`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta profile spam-score USERNAME --field value --flag`

## bookmarks

Bookmark creation, removal, and listing.

### `bookmarks add`

- Command: `./tweeta bookmarks add [--field value ...]`
- HTTP target: `POST /api/bookmarks/add`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta bookmarks add --field value --count 3 --enabled true`

### `bookmarks remove`

- Command: `./tweeta bookmarks remove [--field value ...]`
- HTTP target: `POST /api/bookmarks/remove`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta bookmarks remove --field value --count 3 --enabled true`

### `bookmarks list`

- Command: `./tweeta bookmarks list [--field value ...]`
- HTTP target: `GET /api/bookmarks/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta bookmarks list --field value --flag`

## blocking

Block/mute operations and status checks.

### `blocking block`

- Command: `./tweeta blocking block [--field value ...]`
- HTTP target: `POST /api/blocking/block`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta blocking block --field value --count 3 --enabled true`

### `blocking unblock`

- Command: `./tweeta blocking unblock [--field value ...]`
- HTTP target: `POST /api/blocking/unblock`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta blocking unblock --field value --count 3 --enabled true`

### `blocking check`

- Command: `./tweeta blocking check USER_ID [--field value ...]`
- HTTP target: `GET /api/blocking/check/:userId`
- Path arguments: 1 positional argument. `USER_ID` fills `:userId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta blocking check USER_ID --field value --flag`

### `blocking mute`

- Command: `./tweeta blocking mute [--field value ...]`
- HTTP target: `POST /api/blocking/mute`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta blocking mute --field value --count 3 --enabled true`

### `blocking unmute`

- Command: `./tweeta blocking unmute [--field value ...]`
- HTTP target: `POST /api/blocking/unmute`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta blocking unmute --field value --count 3 --enabled true`

### `blocking check-mute`

- Command: `./tweeta blocking check-mute USER_ID [--field value ...]`
- HTTP target: `GET /api/blocking/check-mute/:userId`
- Path arguments: 1 positional argument. `USER_ID` fills `:userId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta blocking check-mute USER_ID --field value --flag`

### `blocking causes`

- Command: `./tweeta blocking causes [--field value ...]`
- HTTP target: `GET /api/blocking/causes`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta blocking causes --field value --flag`

## muted

Muted words and muted conversation controls.

### `muted words`

- Command: `./tweeta muted words [--field value ...]`
- HTTP target: `GET /api/muted/words`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta muted words --field value --flag`

### `muted add-word`

- Command: `./tweeta muted add-word [--field value ...]`
- HTTP target: `POST /api/muted/words`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta muted add-word --field value --count 3 --enabled true`

### `muted delete-word`

- Command: `./tweeta muted delete-word ID`
- HTTP target: `DELETE /api/muted/words/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta muted delete-word ID`

### `muted delete-word-by-word`

- Command: `./tweeta muted delete-word-by-word WORD`
- HTTP target: `DELETE /api/muted/words/by-word/:word`
- Path arguments: 1 positional argument. `WORD` fills `:word`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta muted delete-word-by-word WORD`

### `muted mute-conversation`

- Command: `./tweeta muted mute-conversation POST_ID [--field value ...]`
- HTTP target: `POST /api/muted/conversations/:postId`
- Path arguments: 1 positional argument. `POST_ID` fills `:postId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta muted mute-conversation POST_ID --field value --count 3 --enabled true`

### `muted conversations`

- Command: `./tweeta muted conversations [--field value ...]`
- HTTP target: `GET /api/muted/conversations`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta muted conversations --field value --flag`

### `muted conversation-status`

- Command: `./tweeta muted conversation-status POST_ID [--field value ...]`
- HTTP target: `GET /api/muted/conversations/:postId/status`
- Path arguments: 1 positional argument. `POST_ID` fills `:postId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta muted conversation-status POST_ID --field value --flag`

## communities

Community lifecycle, membership, roles, invites, and media.

### `communities create`

- Command: `./tweeta communities create [--field value ...]`
- HTTP target: `POST /api/communities`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities create --field value --count 3 --enabled true`

### `communities get`

- Command: `./tweeta communities get ID [--field value ...]`
- HTTP target: `GET /api/communities/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities get ID --field value --flag`

### `communities list`

- Command: `./tweeta communities list [--field value ...]`
- HTTP target: `GET /api/communities`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities list --field value --flag`

### `communities mine`

- Command: `./tweeta communities mine [--field value ...]`
- HTTP target: `GET /api/communities/user/me`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities mine --field value --flag`

### `communities update`

- Command: `./tweeta communities update ID [--field value ...]`
- HTTP target: `PATCH /api/communities/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities update ID --field value --count 3 --enabled true`

### `communities delete`

- Command: `./tweeta communities delete ID`
- HTTP target: `DELETE /api/communities/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta communities delete ID`

### `communities join`

- Command: `./tweeta communities join ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/join`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities join ID --field value --count 3 --enabled true`

### `communities leave`

- Command: `./tweeta communities leave ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/leave`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities leave ID --field value --count 3 --enabled true`

### `communities members`

- Command: `./tweeta communities members ID [--field value ...]`
- HTTP target: `GET /api/communities/:id/members`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities members ID --field value --flag`

### `communities set-role`

- Command: `./tweeta communities set-role ID USER_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/members/:userId/role`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `USER_ID` fills `:userId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities set-role ID USER_ID --field value --count 3 --enabled true`

### `communities ban`

- Command: `./tweeta communities ban ID USER_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/members/:userId/ban`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `USER_ID` fills `:userId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities ban ID USER_ID --field value --count 3 --enabled true`

### `communities unban`

- Command: `./tweeta communities unban ID USER_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/members/:userId/unban`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `USER_ID` fills `:userId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities unban ID USER_ID --field value --count 3 --enabled true`

### `communities access-mode`

- Command: `./tweeta communities access-mode ID [--field value ...]`
- HTTP target: `PATCH /api/communities/:id/access-mode`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities access-mode ID --field value --count 3 --enabled true`

### `communities join-requests`

- Command: `./tweeta communities join-requests ID [--field value ...]`
- HTTP target: `GET /api/communities/:id/join-requests`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities join-requests ID --field value --flag`

### `communities approve-join-request`

- Command: `./tweeta communities approve-join-request ID REQUEST_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/join-requests/:requestId/approve`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `REQUEST_ID` fills `:requestId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities approve-join-request ID REQUEST_ID --field value --count 3 --enabled true`

### `communities reject-join-request`

- Command: `./tweeta communities reject-join-request ID REQUEST_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/join-requests/:requestId/reject`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `REQUEST_ID` fills `:requestId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities reject-join-request ID REQUEST_ID --field value --count 3 --enabled true`

### `communities for-user`

- Command: `./tweeta communities for-user USER_ID [--field value ...]`
- HTTP target: `GET /api/users/:userId/communities`
- Path arguments: 1 positional argument. `USER_ID` fills `:userId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities for-user USER_ID --field value --flag`

### `communities icon`

- Command: `./tweeta communities icon ID [--file PATH] [PATH]`
- HTTP target: `POST /api/communities/:id/icon`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Multipart upload route
- Multipart field name: `icon`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta communities icon ID --file ./icon.png`

### `communities banner`

- Command: `./tweeta communities banner ID [--file PATH] [PATH]`
- HTTP target: `POST /api/communities/:id/banner`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Multipart upload route
- Multipart field name: `banner`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta communities banner ID --file ./banner.png`

### `communities tweets`

- Command: `./tweeta communities tweets ID [--field value ...]`
- HTTP target: `GET /api/communities/:id/tweets`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities tweets ID --field value --flag`

### `communities tag`

- Command: `./tweeta communities tag ID [--field value ...]`
- HTTP target: `PATCH /api/communities/:id/tag`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities tag ID --field value --count 3 --enabled true`

### `communities pin-tweet`

- Command: `./tweeta communities pin-tweet ID TWEET_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/tweets/:tweetId/pin`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `TWEET_ID` fills `:tweetId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities pin-tweet ID TWEET_ID --field value --count 3 --enabled true`

### `communities unpin-tweet`

- Command: `./tweeta communities unpin-tweet ID TWEET_ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/tweets/:tweetId/unpin`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `TWEET_ID` fills `:tweetId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities unpin-tweet ID TWEET_ID --field value --count 3 --enabled true`

### `communities search`

- Command: `./tweeta communities search [--field value ...]`
- HTTP target: `GET /api/communities/search`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities search --field value --flag`

### `communities trending`

- Command: `./tweeta communities trending [--field value ...]`
- HTTP target: `GET /api/communities/trending`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities trending --field value --flag`

### `communities recommended`

- Command: `./tweeta communities recommended [--field value ...]`
- HTTP target: `GET /api/communities/recommended`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities recommended --field value --flag`

### `communities mod-log`

- Command: `./tweeta communities mod-log ID [--field value ...]`
- HTTP target: `GET /api/communities/:id/mod-log`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities mod-log ID --field value --flag`

### `communities create-invite`

- Command: `./tweeta communities create-invite ID [--field value ...]`
- HTTP target: `POST /api/communities/:id/invites`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities create-invite ID --field value --count 3 --enabled true`

### `communities accept-invite`

- Command: `./tweeta communities accept-invite TOKEN [--field value ...]`
- HTTP target: `POST /api/communities/invites/:token/accept`
- Path arguments: 1 positional argument. `TOKEN` fills `:token`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta communities accept-invite TOKEN --field value --count 3 --enabled true`

### `communities invites`

- Command: `./tweeta communities invites ID [--field value ...]`
- HTTP target: `GET /api/communities/:id/invites`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta communities invites ID --field value --flag`

### `communities delete-invite`

- Command: `./tweeta communities delete-invite ID INVITE_ID`
- HTTP target: `DELETE /api/communities/:id/invites/:inviteId`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `INVITE_ID` fills `:inviteId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta communities delete-invite ID INVITE_ID`

## delegates

Delegate invitation and delegation-state management.

### `delegates invite`

- Command: `./tweeta delegates invite [--field value ...]`
- HTTP target: `POST /api/delegates/invite`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta delegates invite --field value --count 3 --enabled true`

### `delegates accept`

- Command: `./tweeta delegates accept ID [--field value ...]`
- HTTP target: `POST /api/delegates/:id/accept`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta delegates accept ID --field value --count 3 --enabled true`

### `delegates decline`

- Command: `./tweeta delegates decline ID [--field value ...]`
- HTTP target: `POST /api/delegates/:id/decline`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta delegates decline ID --field value --count 3 --enabled true`

### `delegates delete`

- Command: `./tweeta delegates delete ID`
- HTTP target: `DELETE /api/delegates/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta delegates delete ID`

### `delegates summary`

- Command: `./tweeta delegates summary [--field value ...]`
- HTTP target: `GET /api/delegates/summary`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta delegates summary --field value --flag`

### `delegates my-delegates`

- Command: `./tweeta delegates my-delegates [--field value ...]`
- HTTP target: `GET /api/delegates/my-delegates`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta delegates my-delegates --field value --flag`

### `delegates my-delegations`

- Command: `./tweeta delegates my-delegations [--field value ...]`
- HTTP target: `GET /api/delegates/my-delegations`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta delegates my-delegations --field value --flag`

### `delegates pending-invitations`

- Command: `./tweeta delegates pending-invitations [--field value ...]`
- HTTP target: `GET /api/delegates/pending-invitations`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta delegates pending-invitations --field value --flag`

### `delegates sent-invitations`

- Command: `./tweeta delegates sent-invitations [--field value ...]`
- HTTP target: `GET /api/delegates/sent-invitations`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta delegates sent-invitations --field value --flag`

### `delegates check`

- Command: `./tweeta delegates check USER_ID [--field value ...]`
- HTTP target: `GET /api/delegates/check/:userId`
- Path arguments: 1 positional argument. `USER_ID` fills `:userId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta delegates check USER_ID --field value --flag`

## dm

Direct-message conversation, membership, message, invite, and pin operations.

### `dm conversations`

- Command: `./tweeta dm conversations [--field value ...]`
- HTTP target: `GET /api/dm/conversations`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta dm conversations --field value --flag`

### `dm conversation`

- Command: `./tweeta dm conversation ID [--field value ...]`
- HTTP target: `GET /api/dm/conversations/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta dm conversation ID --field value --flag`

### `dm create-conversation`

- Command: `./tweeta dm create-conversation [--field value ...]`
- HTTP target: `POST /api/dm/conversations`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm create-conversation --field value --count 3 --enabled true`

### `dm send`

- Command: `./tweeta dm send ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/messages`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm send ID --field value --count 3 --enabled true`

### `dm read`

- Command: `./tweeta dm read ID [--field value ...]`
- HTTP target: `PATCH /api/dm/conversations/:id/read`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm read ID --field value --count 3 --enabled true`

### `dm add-participant`

- Command: `./tweeta dm add-participant ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/participants`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm add-participant ID --field value --count 3 --enabled true`

### `dm remove-participant`

- Command: `./tweeta dm remove-participant ID USER_ID`
- HTTP target: `DELETE /api/dm/conversations/:id/participants/:userId`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `USER_ID` fills `:userId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta dm remove-participant ID USER_ID`

### `dm title`

- Command: `./tweeta dm title ID [--field value ...]`
- HTTP target: `PATCH /api/dm/conversations/:id/title`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm title ID --field value --count 3 --enabled true`

### `dm react`

- Command: `./tweeta dm react MESSAGE_ID [--field value ...]`
- HTTP target: `POST /api/dm/messages/:messageId/reactions`
- Path arguments: 1 positional argument. `MESSAGE_ID` fills `:messageId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm react MESSAGE_ID --field value --count 3 --enabled true`

### `dm typing`

- Command: `./tweeta dm typing ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/typing`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm typing ID --field value --count 3 --enabled true`

### `dm typing-stop`

- Command: `./tweeta dm typing-stop ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/typing-stop`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm typing-stop ID --field value --count 3 --enabled true`

### `dm edit`

- Command: `./tweeta dm edit MESSAGE_ID [--field value ...]`
- HTTP target: `PUT /api/dm/messages/:messageId`
- Path arguments: 1 positional argument. `MESSAGE_ID` fills `:messageId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm edit MESSAGE_ID --field value --count 3 --enabled true`

### `dm delete-message`

- Command: `./tweeta dm delete-message MESSAGE_ID`
- HTTP target: `DELETE /api/dm/messages/:messageId`
- Path arguments: 1 positional argument. `MESSAGE_ID` fills `:messageId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta dm delete-message MESSAGE_ID`

### `dm disappearing`

- Command: `./tweeta dm disappearing ID [--field value ...]`
- HTTP target: `PATCH /api/dm/conversations/:id/disappearing`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm disappearing ID --field value --count 3 --enabled true`

### `dm invite`

- Command: `./tweeta dm invite ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/invite`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm invite ID --field value --count 3 --enabled true`

### `dm revoke-invite`

- Command: `./tweeta dm revoke-invite ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/invite/revoke`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm revoke-invite ID --field value --count 3 --enabled true`

### `dm join`

- Command: `./tweeta dm join TOKEN [--field value ...]`
- HTTP target: `POST /api/dm/conversations/join/:token`
- Path arguments: 1 positional argument. `TOKEN` fills `:token`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm join TOKEN --field value --count 3 --enabled true`

### `dm permissions`

- Command: `./tweeta dm permissions ID [--field value ...]`
- HTTP target: `PATCH /api/dm/conversations/:id/permissions`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm permissions ID --field value --count 3 --enabled true`

### `dm participant-role`

- Command: `./tweeta dm participant-role ID USER_ID [--field value ...]`
- HTTP target: `PATCH /api/dm/conversations/:id/participants/:userId/role`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `USER_ID` fills `:userId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm participant-role ID USER_ID --field value --count 3 --enabled true`

### `dm pin`

- Command: `./tweeta dm pin ID MESSAGE_ID [--field value ...]`
- HTTP target: `POST /api/dm/conversations/:id/pin/:messageId`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `MESSAGE_ID` fills `:messageId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta dm pin ID MESSAGE_ID --field value --count 3 --enabled true`

### `dm unpin`

- Command: `./tweeta dm unpin ID MESSAGE_ID`
- HTTP target: `DELETE /api/dm/conversations/:id/pin/:messageId`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `MESSAGE_ID` fills `:messageId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta dm unpin ID MESSAGE_ID`

### `dm pinned`

- Command: `./tweeta dm pinned ID [--field value ...]`
- HTTP target: `GET /api/dm/conversations/:id/pinned`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta dm pinned ID --field value --flag`

## notifications

Notification listing and read-state updates.

### `notifications list`

- Command: `./tweeta notifications list [--field value ...]`
- HTTP target: `GET /api/notifications/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta notifications list --field value --flag`

### `notifications unread-count`

- Command: `./tweeta notifications unread-count [--field value ...]`
- HTTP target: `GET /api/notifications/unread-count`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta notifications unread-count --field value --flag`

### `notifications read`

- Command: `./tweeta notifications read ID [--field value ...]`
- HTTP target: `PATCH /api/notifications/:id/read`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta notifications read ID --field value --count 3 --enabled true`

### `notifications mark-all-read`

- Command: `./tweeta notifications mark-all-read [--field value ...]`
- HTTP target: `PATCH /api/notifications/mark-all-read`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta notifications mark-all-read --field value --count 3 --enabled true`

## push

Push subscription registration/status routes.

### `push vapid-key`

- Command: `./tweeta push vapid-key [--field value ...]`
- HTTP target: `GET /api/push/vapid-key`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta push vapid-key --field value --flag`

### `push subscribe`

- Command: `./tweeta push subscribe [--field value ...]`
- HTTP target: `POST /api/push/subscribe`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta push subscribe --field value --count 3 --enabled true`

### `push unsubscribe`

- Command: `./tweeta push unsubscribe [--field value ...]`
- HTTP target: `POST /api/push/unsubscribe`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta push unsubscribe --field value --count 3 --enabled true`

### `push status`

- Command: `./tweeta push status [--field value ...]`
- HTTP target: `GET /api/push/status`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta push status --field value --flag`

## lists

User list CRUD, membership, and follow operations.

### `lists list`

- Command: `./tweeta lists list [--field value ...]`
- HTTP target: `GET /api/lists/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists list --field value --flag`

### `lists for-user`

- Command: `./tweeta lists for-user USERNAME [--field value ...]`
- HTTP target: `GET /api/lists/user/:username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists for-user USERNAME --field value --flag`

### `lists containing`

- Command: `./tweeta lists containing USERNAME [--field value ...]`
- HTTP target: `GET /api/lists/containing/:username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists containing USERNAME --field value --flag`

### `lists create`

- Command: `./tweeta lists create [--field value ...]`
- HTTP target: `POST /api/lists/`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta lists create --field value --count 3 --enabled true`

### `lists get`

- Command: `./tweeta lists get ID [--field value ...]`
- HTTP target: `GET /api/lists/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists get ID --field value --flag`

### `lists update`

- Command: `./tweeta lists update ID [--field value ...]`
- HTTP target: `PATCH /api/lists/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta lists update ID --field value --count 3 --enabled true`

### `lists delete`

- Command: `./tweeta lists delete ID`
- HTTP target: `DELETE /api/lists/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta lists delete ID`

### `lists tweets`

- Command: `./tweeta lists tweets ID [--field value ...]`
- HTTP target: `GET /api/lists/:id/tweets`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists tweets ID --field value --flag`

### `lists add-member`

- Command: `./tweeta lists add-member ID [--field value ...]`
- HTTP target: `POST /api/lists/:id/members`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta lists add-member ID --field value --count 3 --enabled true`

### `lists remove-member`

- Command: `./tweeta lists remove-member ID USER_ID`
- HTTP target: `DELETE /api/lists/:id/members/:userId`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `USER_ID` fills `:userId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta lists remove-member ID USER_ID`

### `lists follow`

- Command: `./tweeta lists follow ID [--field value ...]`
- HTTP target: `POST /api/lists/:id/follow`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta lists follow ID --field value --count 3 --enabled true`

### `lists unfollow`

- Command: `./tweeta lists unfollow ID`
- HTTP target: `DELETE /api/lists/:id/follow`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta lists unfollow ID`

### `lists members`

- Command: `./tweeta lists members ID [--field value ...]`
- HTTP target: `GET /api/lists/:id/members`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists members ID --field value --flag`

### `lists followers`

- Command: `./tweeta lists followers ID [--field value ...]`
- HTTP target: `GET /api/lists/:id/followers`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta lists followers ID --field value --flag`

## articles

Article creation and retrieval routes.

### `articles create`

- Command: `./tweeta articles create [--field value ...]`
- HTTP target: `POST /api/articles/`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta articles create --field value --count 3 --enabled true`

### `articles list`

- Command: `./tweeta articles list [--field value ...]`
- HTTP target: `GET /api/articles/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta articles list --field value --flag`

### `articles get`

- Command: `./tweeta articles get ID [--field value ...]`
- HTTP target: `GET /api/articles/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta articles get ID --field value --flag`

## search

Search routes for users and posts.

### `search users`

- Command: `./tweeta search users [--field value ...]`
- HTTP target: `GET /api/search/users`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta search users --field value --flag`

### `search posts`

- Command: `./tweeta search posts [--field value ...]`
- HTTP target: `GET /api/search/posts`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta search posts --field value --flag`

## public-tweets

Public tweet feed access.

### `public-tweets list`

- Command: `./tweeta public-tweets list [--field value ...]`
- HTTP target: `GET /api/public-tweets/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta public-tweets list --field value --flag`

## scheduled

Scheduled post creation, listing, and deletion.

### `scheduled create`

- Command: `./tweeta scheduled create [--field value ...]`
- HTTP target: `POST /api/scheduled/`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta scheduled create --field value --count 3 --enabled true`

### `scheduled list`

- Command: `./tweeta scheduled list [--field value ...]`
- HTTP target: `GET /api/scheduled/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta scheduled list --field value --flag`

### `scheduled delete`

- Command: `./tweeta scheduled delete ID`
- HTTP target: `DELETE /api/scheduled/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta scheduled delete ID`

## reports

Report creation.

### `reports create`

- Command: `./tweeta reports create [--field value ...]`
- HTTP target: `POST /api/reports/create`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta reports create --field value --count 3 --enabled true`

## translate

Translation requests.

### `translate translate`

- Command: `./tweeta translate translate [--field value ...]`
- HTTP target: `POST /api/translate/`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta translate translate --field value --count 3 --enabled true`

## trends

Trend listing.

### `trends list`

- Command: `./tweeta trends list [--field value ...]`
- HTTP target: `GET /api/trends/`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta trends list --field value --flag`

## tenor

Tenor search proxy route.

### `tenor search`

- Command: `./tweeta tenor search [--field value ...]`
- HTTP target: `GET /api/tenor/search`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta tenor search --field value --flag`

## unsplash

Unsplash search proxy route.

### `unsplash search`

- Command: `./tweeta unsplash search [--field value ...]`
- HTTP target: `GET /api/unsplash/search`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta unsplash search --field value --flag`

## sse

Server-sent event connection route.

### `sse connect`

- Command: `./tweeta sse connect [--field value ...]`
- HTTP target: `GET /api/sse`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta sse connect --field value --flag`

## upload

Raw upload and uploaded-file retrieval routes.

### `upload media`

- Command: `./tweeta upload media [--file PATH] [PATH]`
- HTTP target: `POST /api/upload`
- Path arguments: None.
- Request shape: Multipart upload route
- Multipart field name: `file`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta upload media --file ./upload.bin`
- Notes: Shortcut alias: `./tweeta upload media FILE` is implemented directly in `main.c` and is equivalent to the named route when `FILE` is the upload path.

### `upload get`

- Command: `./tweeta upload get FILENAME [--field value ...]`
- HTTP target: `GET /api/uploads/:filename`
- Path arguments: 1 positional argument. `FILENAME` fills `:filename`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta upload get FILENAME --field value --flag`
- Notes: Collision note: `./tweeta upload get FILENAME` uses this named route, but `./tweeta upload get --id POST_ID [--all] [--really-all]` is intercepted earlier by the attachment-download helper and does something different. If you already have the upload filename or relative upload path, this named route is the direct way to fetch it. If you only have a post ID, use the attachment helper form instead.

## explore

Explore/discovery routes and analytics-style reads.

### `explore best-of-week`

- Command: `./tweeta explore best-of-week [--field value ...]`
- HTTP target: `GET /api/explore/best-of-week`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore best-of-week --field value --flag`

### `explore trending-users`

- Command: `./tweeta explore trending-users [--field value ...]`
- HTTP target: `GET /api/explore/trending-users`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore trending-users --field value --flag`

### `explore most-bookmarked`

- Command: `./tweeta explore most-bookmarked [--field value ...]`
- HTTP target: `GET /api/explore/most-bookmarked`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore most-bookmarked --field value --flag`

### `explore most-discussed`

- Command: `./tweeta explore most-discussed [--field value ...]`
- HTTP target: `GET /api/explore/most-discussed`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore most-discussed --field value --flag`

### `explore longest-threads`

- Command: `./tweeta explore longest-threads [--field value ...]`
- HTTP target: `GET /api/explore/longest-threads`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore longest-threads --field value --flag`

### `explore with-media`

- Command: `./tweeta explore with-media [--field value ...]`
- HTTP target: `GET /api/explore/with-media`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore with-media --field value --flag`

### `explore with-polls`

- Command: `./tweeta explore with-polls [--field value ...]`
- HTTP target: `GET /api/explore/with-polls`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore with-polls --field value --flag`

### `explore digest`

- Command: `./tweeta explore digest [--field value ...]`
- HTTP target: `GET /api/explore/digest`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore digest --field value --flag`

### `explore top-hashtags`

- Command: `./tweeta explore top-hashtags [--field value ...]`
- HTTP target: `GET /api/explore/top-hashtags`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore top-hashtags --field value --flag`

### `explore leaderboard`

- Command: `./tweeta explore leaderboard [--field value ...]`
- HTTP target: `GET /api/explore/leaderboard`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore leaderboard --field value --flag`

### `explore suggested-users`

- Command: `./tweeta explore suggested-users [--field value ...]`
- HTTP target: `GET /api/explore/suggested-users`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore suggested-users --field value --flag`

### `explore directory`

- Command: `./tweeta explore directory [--field value ...]`
- HTTP target: `GET /api/explore/directory`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore directory --field value --flag`

### `explore user-analytics`

- Command: `./tweeta explore user-analytics USERNAME [--field value ...]`
- HTTP target: `GET /api/explore/users/:username/analytics`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore user-analytics USERNAME --field value --flag`

### `explore common-followers`

- Command: `./tweeta explore common-followers USERNAME [--field value ...]`
- HTTP target: `GET /api/explore/users/:username/common-followers`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore common-followers USERNAME --field value --flag`

### `explore top-posts`

- Command: `./tweeta explore top-posts USERNAME [--field value ...]`
- HTTP target: `GET /api/explore/users/:username/top-posts`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore top-posts USERNAME --field value --flag`

### `explore stats`

- Command: `./tweeta explore stats [--field value ...]`
- HTTP target: `GET /api/explore/stats`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta explore stats --field value --flag`

## mpi

MPI payment/request message operations.

### `mpi send-create`

- Command: `./tweeta mpi send-create [--field value ...]`
- HTTP target: `POST /api/mpi/send/create`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta mpi send-create --field value --count 3 --enabled true`

### `mpi send-confirm`

- Command: `./tweeta mpi send-confirm [--field value ...]`
- HTTP target: `POST /api/mpi/send/confirm`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta mpi send-confirm --field value --count 3 --enabled true`

### `mpi request`

- Command: `./tweeta mpi request [--field value ...]`
- HTTP target: `POST /api/mpi/request`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta mpi request --field value --count 3 --enabled true`

### `mpi request-pay`

- Command: `./tweeta mpi request-pay MESSAGE_ID [--field value ...]`
- HTTP target: `POST /api/mpi/request/:messageId/pay`
- Path arguments: 1 positional argument. `MESSAGE_ID` fills `:messageId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta mpi request-pay MESSAGE_ID --field value --count 3 --enabled true`

### `mpi request-confirm`

- Command: `./tweeta mpi request-confirm MESSAGE_ID [--field value ...]`
- HTTP target: `POST /api/mpi/request/:messageId/confirm`
- Path arguments: 1 positional argument. `MESSAGE_ID` fills `:messageId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta mpi request-confirm MESSAGE_ID --field value --count 3 --enabled true`

### `mpi payments-by-message`

- Command: `./tweeta mpi payments-by-message MESSAGE_ID [--field value ...]`
- HTTP target: `GET /api/mpi/payments/by-message/:messageId`
- Path arguments: 1 positional argument. `MESSAGE_ID` fills `:messageId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta mpi payments-by-message MESSAGE_ID --field value --flag`

## shop

Shop/catalog/product/purchase operations under the MPI namespace.

### `shop user`

- Command: `./tweeta shop user USERNAME [--field value ...]`
- HTTP target: `GET /api/mpi/shop/user/:username`
- Path arguments: 1 positional argument. `USERNAME` fills `:username`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta shop user USERNAME --field value --flag`

### `shop product`

- Command: `./tweeta shop product ID [--field value ...]`
- HTTP target: `GET /api/mpi/shop/product/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta shop product ID --field value --flag`

### `shop create-product`

- Command: `./tweeta shop create-product [--field value ...]`
- HTTP target: `POST /api/mpi/shop/product`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta shop create-product --field value --count 3 --enabled true`

### `shop update-product`

- Command: `./tweeta shop update-product ID [--field value ...]`
- HTTP target: `PATCH /api/mpi/shop/product/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta shop update-product ID --field value --count 3 --enabled true`

### `shop delete-product`

- Command: `./tweeta shop delete-product ID`
- HTTP target: `DELETE /api/mpi/shop/product/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta shop delete-product ID`

### `shop buy`

- Command: `./tweeta shop buy ID [--field value ...]`
- HTTP target: `POST /api/mpi/shop/product/:id/buy`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta shop buy ID --field value --count 3 --enabled true`

### `shop confirm`

- Command: `./tweeta shop confirm ID [--field value ...]`
- HTTP target: `POST /api/mpi/shop/product/:id/confirm`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta shop confirm ID --field value --count 3 --enabled true`

### `shop purchase`

- Command: `./tweeta shop purchase ID [--field value ...]`
- HTTP target: `GET /api/mpi/shop/purchase/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta shop purchase ID --field value --flag`

## admin

Admin-only moderation, user, post, DM, fact-check, IP, badge, and shop routes.

### `admin stats`

- Command: `./tweeta admin stats [--field value ...]`
- HTTP target: `GET /api/admin/stats`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin stats --field value --flag`
- Notes: Legacy alias: `./tweeta admin stats` is also handled by a shortcut helper.

### `admin users`

- Command: `./tweeta admin users [--field value ...]`
- HTTP target: `GET /api/admin/users`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin users --field value --flag`
- Notes: Legacy alias: `./tweeta admin users [--field value ...]` is also handled by a shortcut helper.

### `admin create-user`

- Command: `./tweeta admin create-user [--field value ...]`
- HTTP target: `POST /api/admin/users`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-user --field value --count 3 --enabled true`

### `admin create-affiliate-request`

- Command: `./tweeta admin create-affiliate-request ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/affiliate-requests`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-affiliate-request ID --field value --count 3 --enabled true`

### `admin approve-affiliate-request`

- Command: `./tweeta admin approve-affiliate-request ID [--field value ...]`
- HTTP target: `POST /api/admin/affiliate-requests/:id/approve`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin approve-affiliate-request ID --field value --count 3 --enabled true`

### `admin deny-affiliate-request`

- Command: `./tweeta admin deny-affiliate-request ID [--field value ...]`
- HTTP target: `POST /api/admin/affiliate-requests/:id/deny`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin deny-affiliate-request ID --field value --count 3 --enabled true`

### `admin user`

- Command: `./tweeta admin user ID [--field value ...]`
- HTTP target: `GET /api/admin/users/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin user ID --field value --flag`
- Notes: Legacy alias: `./tweeta admin user ID` is also handled by a shortcut helper.

### `admin verify`

- Command: `./tweeta admin verify ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id/verify`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin verify ID --field value --count 3 --enabled true`

### `admin gold`

- Command: `./tweeta admin gold ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id/gold`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin gold ID --field value --count 3 --enabled true`

### `admin gray`

- Command: `./tweeta admin gray ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id/gray`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin gray ID --field value --count 3 --enabled true`

### `admin outlines`

- Command: `./tweeta admin outlines ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id/outlines`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin outlines ID --field value --count 3 --enabled true`

### `admin set-avatar`

- Command: `./tweeta admin set-avatar ID [--file PATH] [PATH]`
- HTTP target: `POST /api/admin/users/:id/avatar`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Multipart upload route
- Multipart field name: `avatar`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta admin set-avatar ID --file ./avatar.png`

### `admin delete-avatar`

- Command: `./tweeta admin delete-avatar ID`
- HTTP target: `DELETE /api/admin/users/:id/avatar`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-avatar ID`

### `admin set-banner`

- Command: `./tweeta admin set-banner ID [--file PATH] [PATH]`
- HTTP target: `POST /api/admin/users/:id/banner`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Multipart upload route
- Multipart field name: `banner`
- CLI behavior: This route sends multipart/form-data. Supply the upload path as `--file PATH` or as the first non-flag argument immediately after the required positional arguments. The CLI always uses the multipart field name shown in this section; the field name is fixed per route.
- Example: `./tweeta admin set-banner ID --file ./banner.png`

### `admin delete-banner`

- Command: `./tweeta admin delete-banner ID`
- HTTP target: `DELETE /api/admin/users/:id/banner`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-banner ID`

### `admin mass-follow`

- Command: `./tweeta admin mass-follow ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/mass-follow`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin mass-follow ID --field value --count 3 --enabled true`

### `admin permissions`

- Command: `./tweeta admin permissions ID [--field value ...]`
- HTTP target: `GET /api/admin/users/:id/permissions`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin permissions ID --field value --flag`

### `admin set-permissions`

- Command: `./tweeta admin set-permissions ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id/permissions`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin set-permissions ID --field value --count 3 --enabled true`

### `admin badges`

- Command: `./tweeta admin badges [--field value ...]`
- HTTP target: `GET /api/admin/badges`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin badges --field value --flag`

### `admin create-badge`

- Command: `./tweeta admin create-badge [--field value ...]`
- HTTP target: `POST /api/admin/badges`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-badge --field value --count 3 --enabled true`

### `admin update-badge`

- Command: `./tweeta admin update-badge ID [--field value ...]`
- HTTP target: `PATCH /api/admin/badges/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin update-badge ID --field value --count 3 --enabled true`

### `admin delete-badge`

- Command: `./tweeta admin delete-badge ID`
- HTTP target: `DELETE /api/admin/badges/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-badge ID`

### `admin user-badges`

- Command: `./tweeta admin user-badges ID [--field value ...]`
- HTTP target: `GET /api/admin/users/:id/badges`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin user-badges ID --field value --flag`

### `admin assign-badge`

- Command: `./tweeta admin assign-badge ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/badges`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin assign-badge ID --field value --count 3 --enabled true`

### `admin remove-badge`

- Command: `./tweeta admin remove-badge ID BADGE_ID`
- HTTP target: `DELETE /api/admin/users/:id/badges/:badgeId`
- Path arguments: 2 positional arguments, in order: `ID` fills `:id`; `BADGE_ID` fills `:badgeId`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin remove-badge ID BADGE_ID`

### `admin suspend`

- Command: `./tweeta admin suspend ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/suspend`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin suspend ID --field value --count 3 --enabled true`
- Notes: Legacy alias: `./tweeta admin suspend ID --reason TEXT [--action ACTION] [--duration MINUTES] [--notes TEXT]` is also handled by a shortcut helper. The named route documented here goes through the generic JSON-body path instead.

### `admin unsuspend`

- Command: `./tweeta admin unsuspend ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/unsuspend`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin unsuspend ID --field value --count 3 --enabled true`
- Notes: Legacy alias: `./tweeta admin unsuspend ID` is also handled by a shortcut helper.

### `admin delete-user`

- Command: `./tweeta admin delete-user ID`
- HTTP target: `DELETE /api/admin/users/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-user ID`

### `admin clone-user`

- Command: `./tweeta admin clone-user ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/clone`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin clone-user ID --field value --count 3 --enabled true`

### `admin posts`

- Command: `./tweeta admin posts [--field value ...]`
- HTTP target: `GET /api/admin/posts`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin posts --field value --flag`

### `admin delete-post`

- Command: `./tweeta admin delete-post ID`
- HTTP target: `DELETE /api/admin/posts/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-post ID`

### `admin suspensions`

- Command: `./tweeta admin suspensions [--field value ...]`
- HTTP target: `GET /api/admin/suspensions`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin suspensions --field value --flag`

### `admin post`

- Command: `./tweeta admin post ID [--field value ...]`
- HTTP target: `GET /api/admin/posts/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin post ID --field value --flag`

### `admin update-post`

- Command: `./tweeta admin update-post ID [--field value ...]`
- HTTP target: `PATCH /api/admin/posts/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin update-post ID --field value --count 3 --enabled true`

### `admin change-post-id`

- Command: `./tweeta admin change-post-id ID [--field value ...]`
- HTTP target: `PATCH /api/admin/posts/:id/id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin change-post-id ID --field value --count 3 --enabled true`

### `admin create-tweet`

- Command: `./tweeta admin create-tweet [--field value ...]`
- HTTP target: `POST /api/admin/tweets`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-tweet --field value --count 3 --enabled true`

### `admin update-user`

- Command: `./tweeta admin update-user ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin update-user ID --field value --count 3 --enabled true`

### `admin impersonate`

- Command: `./tweeta admin impersonate ID [--field value ...]`
- HTTP target: `POST /api/admin/impersonate/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin impersonate ID --field value --count 3 --enabled true`

### `admin dms`

- Command: `./tweeta admin dms [--field value ...]`
- HTTP target: `GET /api/admin/dms`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin dms --field value --flag`

### `admin search-dms`

- Command: `./tweeta admin search-dms [--field value ...]`
- HTTP target: `GET /api/admin/dms/search`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin search-dms --field value --flag`

### `admin dm`

- Command: `./tweeta admin dm ID [--field value ...]`
- HTTP target: `GET /api/admin/dms/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin dm ID --field value --flag`

### `admin dm-messages`

- Command: `./tweeta admin dm-messages ID [--field value ...]`
- HTTP target: `GET /api/admin/dms/:id/messages`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin dm-messages ID --field value --flag`

### `admin delete-dm`

- Command: `./tweeta admin delete-dm ID`
- HTTP target: `DELETE /api/admin/dms/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-dm ID`

### `admin delete-dm-message`

- Command: `./tweeta admin delete-dm-message ID`
- HTTP target: `DELETE /api/admin/dms/messages/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-dm-message ID`

### `admin fake-notification`

- Command: `./tweeta admin fake-notification [--field value ...]`
- HTTP target: `POST /api/admin/fake-notification`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin fake-notification --field value --count 3 --enabled true`

### `admin moderation-logs`

- Command: `./tweeta admin moderation-logs [--field value ...]`
- HTTP target: `GET /api/admin/moderation-logs`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin moderation-logs --field value --flag`

### `admin target-logs`

- Command: `./tweeta admin target-logs ID [--field value ...]`
- HTTP target: `GET /api/admin/moderation-logs/target/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin target-logs ID --field value --flag`

### `admin moderator-logs`

- Command: `./tweeta admin moderator-logs ID [--field value ...]`
- HTTP target: `GET /api/admin/moderation-logs/moderator/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin moderator-logs ID --field value --flag`

### `admin emojis`

- Command: `./tweeta admin emojis [--field value ...]`
- HTTP target: `GET /api/admin/emojis`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin emojis --field value --flag`

### `admin create-emoji`

- Command: `./tweeta admin create-emoji [--field value ...]`
- HTTP target: `POST /api/admin/emojis`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-emoji --field value --count 3 --enabled true`

### `admin delete-emoji`

- Command: `./tweeta admin delete-emoji ID`
- HTTP target: `DELETE /api/admin/emojis/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-emoji ID`

### `admin fact-check`

- Command: `./tweeta admin fact-check POST_ID [--field value ...]`
- HTTP target: `GET /api/admin/fact-check/:postId`
- Path arguments: 1 positional argument. `POST_ID` fills `:postId`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin fact-check POST_ID --field value --flag`

### `admin create-fact-check`

- Command: `./tweeta admin create-fact-check POST_ID [--field value ...]`
- HTTP target: `POST /api/admin/fact-check/:postId`
- Path arguments: 1 positional argument. `POST_ID` fills `:postId`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-fact-check POST_ID --field value --count 3 --enabled true`

### `admin delete-fact-check`

- Command: `./tweeta admin delete-fact-check ID`
- HTTP target: `DELETE /api/admin/fact-check/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-fact-check ID`

### `admin reports`

- Command: `./tweeta admin reports [--field value ...]`
- HTTP target: `GET /api/admin/reports`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin reports --field value --flag`

### `admin resolve-report`

- Command: `./tweeta admin resolve-report ID [--field value ...]`
- HTTP target: `POST /api/admin/reports/:id/resolve`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin resolve-report ID --field value --count 3 --enabled true`

### `admin super-tweeter`

- Command: `./tweeta admin super-tweeter ID [--field value ...]`
- HTTP target: `PATCH /api/admin/users/:id/super-tweeter`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin super-tweeter ID --field value --count 3 --enabled true`

### `admin super-tweet`

- Command: `./tweeta admin super-tweet ID [--field value ...]`
- HTTP target: `PATCH /api/admin/posts/:id/super-tweet`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin super-tweet ID --field value --count 3 --enabled true`

### `admin user-ip`

- Command: `./tweeta admin user-ip ID [--field value ...]`
- HTTP target: `GET /api/admin/users/:id/ip`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin user-ip ID --field value --flag`

### `admin ip-users`

- Command: `./tweeta admin ip-users IP [--field value ...]`
- HTTP target: `GET /api/admin/ip/:ip/users`
- Path arguments: 1 positional argument. `IP` fills `:ip`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin ip-users IP --field value --flag`

### `admin ban-ip`

- Command: `./tweeta admin ban-ip [--field value ...]`
- HTTP target: `POST /api/admin/ip/ban`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin ban-ip --field value --count 3 --enabled true`

### `admin unban-ip`

- Command: `./tweeta admin unban-ip [--field value ...]`
- HTTP target: `POST /api/admin/ip/unban`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin unban-ip --field value --count 3 --enabled true`

### `admin ip-bans`

- Command: `./tweeta admin ip-bans [--field value ...]`
- HTTP target: `GET /api/admin/ip/bans`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin ip-bans --field value --flag`

### `admin mass-engage`

- Command: `./tweeta admin mass-engage ID [--field value ...]`
- HTTP target: `POST /api/admin/posts/:id/mass-engage`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin mass-engage ID --field value --count 3 --enabled true`

### `admin mass-delete-posts`

- Command: `./tweeta admin mass-delete-posts [--field value ...]`
- HTTP target: `POST /api/admin/posts/mass-delete`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin mass-delete-posts --field value --count 3 --enabled true`

### `admin user-blocks`

- Command: `./tweeta admin user-blocks ID [--field value ...]`
- HTTP target: `GET /api/admin/users/:id/blocks`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin user-blocks ID --field value --flag`

### `admin user-blocked-by`

- Command: `./tweeta admin user-blocked-by ID [--field value ...]`
- HTTP target: `GET /api/admin/users/:id/blocked-by`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin user-blocked-by ID --field value --flag`

### `admin blocks`

- Command: `./tweeta admin blocks [--field value ...]`
- HTTP target: `GET /api/admin/blocks`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin blocks --field value --flag`

### `admin create-ip-ban`

- Command: `./tweeta admin create-ip-ban [--field value ...]`
- HTTP target: `POST /api/admin/ip-bans`
- Path arguments: None.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin create-ip-ban --field value --count 3 --enabled true`

### `admin rate-limit`

- Command: `./tweeta admin rate-limit ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/rate-limit`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin rate-limit ID --field value --count 3 --enabled true`

### `admin captcha-exempt`

- Command: `./tweeta admin captcha-exempt ID [--field value ...]`
- HTTP target: `POST /api/admin/users/:id/captcha-exempt`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: JSON body route
- CLI behavior: After the positional arguments, every `--name value` pair becomes a JSON object field. The CLI auto-types literal `true`, `false`, `null`, and numeric values, and it passes values beginning with `{` or `[` through as raw JSON. Everything else is sent as a JSON string. A bare flag with no value becomes `true`.
- Example: `./tweeta admin captcha-exempt ID --field value --count 3 --enabled true`

### `admin shop-products`

- Command: `./tweeta admin shop-products [--field value ...]`
- HTTP target: `GET /api/admin/shop/products`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin shop-products --field value --flag`

### `admin delete-shop-product`

- Command: `./tweeta admin delete-shop-product ID`
- HTTP target: `DELETE /api/admin/shop/products/:id`
- Path arguments: 1 positional argument. `ID` fills `:id`.
- Request shape: No-payload route
- CLI behavior: This route sends only the method and resolved path. Do not expect trailing `--name value` options to be transmitted; the named-route dispatcher does not build a query string or request body for `PAYLOAD_NONE` routes.
- Example: `./tweeta admin delete-shop-product ID`

### `admin shop-purchases`

- Command: `./tweeta admin shop-purchases [--field value ...]`
- HTTP target: `GET /api/admin/shop/purchases`
- Path arguments: None.
- Request shape: Query route
- CLI behavior: After the positional arguments, every `--name value` pair is appended as a URL query parameter. If you provide a flag with no following value, the CLI sends it as `name=true`. Values are URL-encoded strings; there is no JSON type coercion for query routes.
- Example: `./tweeta admin shop-purchases --field value --flag`

