def create_file(filename, content=b'content'):
    with open(filename, "xb", 0) as fp:
        fp.write(content)
