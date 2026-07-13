#!/usr/bin/env python3
"""Serve the repo over HTTP with CORS headers (default port 8642).

Used by the browser-automation IDE workflow: page-side JavaScript in
the web64 IDE can fetch() repo files (sources to inject into virtual
files, .web64proj files to feed the project-open file input) without
pasting file contents through other channels.
"""

import functools
import http.server
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8642


class CORSHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()


if __name__ == "__main__":
    print(f"serving {ROOT} on http://127.0.0.1:{PORT} (Ctrl-C to stop)")
    http.server.HTTPServer(
        ("127.0.0.1", PORT), functools.partial(CORSHandler, directory=str(ROOT))
    ).serve_forever()
