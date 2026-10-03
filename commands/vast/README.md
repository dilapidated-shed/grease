# Vast.ai from Grease

`vast.ysh` is a thin Grease/YSH client for the Vast.ai public REST API. It
deliberately does not import Vast's Python SDK: Grease can reach the provider
through the ordinary HTTP boundary with `curl`, while provider-specific paths
remain isolated in one command.

The current companion API inventory is maintained on the `vast` branch of
`fuego-ironworks/gym` and in its PR #60, **Document Vast.ai public compute
API**. That note records the current mixed v0/v1 API surface and billing
semantics.

## Authentication

Put the Vast key in the environment:

```sh
export VAST_API_KEY='...'
```

Do not pass the key as a positional argument or commit it.

The production base URL defaults to:

```text
https://console.vast.ai
```

`VAST_API_BASE_URL` can override that host for tests or a future compatible
gateway.

## Commands

Run through the Grease/YSH executable:

```sh
ysh commands/vast/vast.ysh user

ysh commands/vast/vast.ysh offers \
  '{"limit":5,"verified":{"eq":true},"rentable":{"eq":true}}'

ysh commands/vast/vast.ysh create 123456 \
  '{"image":"pytorch/pytorch:latest","disk":40,"runtype":"ssh","target_state":"running"}'

ysh commands/vast/vast.ysh instance 789012
ysh commands/vast/vast.ysh instances
ysh commands/vast/vast.ysh instances 'limit=25&select_filters=%7B%7D'
ysh commands/vast/vast.ysh start 789012
ysh commands/vast/vast.ysh stop 789012
ysh commands/vast/vast.ysh destroy 789012
```

The command writes Vast's JSON response to standard output and lets `curl`
surface HTTP failures as nonzero status. Parsing and higher-level job policy do
not belong in this transport shim.

## Current endpoint map

| Grease command | Vast request |
| --- | --- |
| `user` | `GET /api/v0/users/current/` |
| `offers` | `POST /api/v0/bundles/` |
| `create` | `PUT /api/v0/asks/{offer_id}/` |
| `instance` | `GET /api/v0/instances/{instance_id}/` |
| `instances` | `GET /api/v1/instances/` |
| `start` | `PUT /api/v0/instances/{instance_id}/` with `state=running` |
| `stop` | `PUT /api/v0/instances/{instance_id}/` with `state=stopped` |
| `destroy` | `DELETE /api/v0/instances/{instance_id}/` |

The v0/v1 split is intentional because it reflects Vast's current public
OpenAPI definitions. Do not normalize these paths into an invented uniform API.

## Evidence boundary

CI runs this script with the exact pinned Grease/YSH executable against a local
HTTP fixture and checks the request method, path, bearer header, and JSON body.
That proves Grease can form the required HTTP operations without exposing a Vast
credential.

The fixture is not evidence that Vast accepted a live request, that an offer was
available, that an instance started, or that billing behaved as expected. A
credentialed live smoke should remain a separate receipt.
