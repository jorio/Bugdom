#!/usr/bin/env python3
"""Serve the browser build to localhost, simulators, and phones on the LAN."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path


class DevHandler(SimpleHTTPRequestHandler):
    extensions_map = {**SimpleHTTPRequestHandler.extensions_map, '.wasm': 'application/wasm'}

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8000)
    parser.add_argument('--bind', default='0.0.0.0')
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent / 'build-web'
    if not (root / 'index.html').exists():
        parser.error('No browser build. Run bash web/build.sh first.')
    server = ThreadingHTTPServer((args.bind, args.port), partial(DevHandler, directory=str(root)))
    print(f'Bugdom: http://localhost:{args.port} (listening on {args.bind})', flush=True)
    print('Phone: use this computer’s LAN IP. Android emulator: use 10.0.2.2.', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
