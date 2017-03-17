Issue #25931: Don't define socketserver.Forking* names on platforms such
as Windows that do not support os.fork().