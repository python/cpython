Issue #23430: Change the socketserver module to only catch exceptions
raised from a request handler that are derived from Exception (instead of
BaseException).  Therefore SystemExit and KeyboardInterrupt no longer
trigger the handle_error() method, and will now to stop a single-threaded
server.