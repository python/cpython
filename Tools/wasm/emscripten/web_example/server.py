#!/usr/bin/env python
import argparse
from http import server

parser = argparse.ArgumentParser(
    description="Start a local webserver with a Python terminal."
)
parser.add_argument(
    "--port", type=int, default=8000, help="port for the http server to listen on"
)
parser.add_argument(
    "--bind", type=str, default="127.0.0.1", help="Bind address (empty for all)"
)


class MyHTTPRequestHandler(server.SimpleHTTPRequestHandler):
    def end_headers(self) -> None:
        self.send_my_headers()
        super().end_headers()

    def send_my_headers(self) -> None:
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")


def main() -> None:
    args = parser.parse_args()
    if not args.bind:
        args.bind = None

    server.test(  # type: ignore[attr-defined]
        HandlerClass=MyHTTPRequestHandler,
        protocol="HTTP/1.1",
        port=args.port,
        bind=args.bind,
    )


if __name__ == "__main__":
    main()
