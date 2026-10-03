#!/usr/bin/env python3

"""Local HTTP fixture for the Grease Vast.ai client acceptance test."""

import argparse
import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


EXPECTED = [
    ("GET", "/api/v0/users/current/", ""),
    ("POST", "/api/v0/bundles/", '{"limit":1}'),
    ("PUT", "/api/v0/asks/42/", '{"image":"example/test:1","disk":20}'),
    ("GET", "/api/v0/instances/77/", ""),
    ("GET", "/api/v1/instances/?limit=25", ""),
    ("PUT", "/api/v0/instances/77/", '{"state":"running"}'),
    ("PUT", "/api/v0/instances/77/", '{"state":"stopped"}'),
    ("DELETE", "/api/v0/instances/77/", ""),
]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--port-file", required=True)
    args = parser.parse_args()

    requests = []

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, format, *values):
            return

        def do_GET(self):
            self._record()

        def do_POST(self):
            self._record()

        def do_PUT(self):
            self._record()

        def do_DELETE(self):
            self._record()

        def _record(self):
            length = int(self.headers.get("Content-Length", "0"))
            body = self.rfile.read(length).decode("utf-8") if length else ""
            requests.append(
                (
                    self.command,
                    self.path,
                    body,
                    self.headers.get("Authorization"),
                    self.headers.get("Content-Type"),
                )
            )
            payload = json.dumps(
                {"ok": True, "method": self.command, "path": self.path}
            ).encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            self.wfile.write(payload)

    server = HTTPServer(("127.0.0.1", 0), Handler)
    Path(args.port_file).write_text(str(server.server_port), encoding="ascii")

    server.timeout = 30
    while len(requests) < len(EXPECTED):
        before = len(requests)
        server.handle_request()
        if len(requests) == before:
            print(f"FAIL Vast fixture timed out after {len(requests)} requests")
            return 3

    server.server_close()

    actual_core = [(method, path, body) for method, path, body, _, _ in requests]
    if actual_core != EXPECTED:
        print("FAIL Vast request sequence")
        print("expected:", EXPECTED)
        print("actual:", actual_core)
        return 4

    for method, path, body, authorization, content_type in requests:
        if authorization != "Bearer fixture-token":
            print(f"FAIL missing or wrong bearer token for {method} {path}")
            return 5
        if body and content_type != "application/json":
            print(f"FAIL missing JSON content type for {method} {path}")
            return 6

    print("PASS Grease emitted the expected Vast.ai REST requests")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
