#!__VENV_PYTHON__
if __name__ == '__main__':
    rc = 1
    try:
        import sys, re, packaging.run
        sys.argv[0] = re.sub('-script.pyw?$', '', sys.argv[0])
        rc = packaging.run.main() # None interpreted as 0
    except Exception:
        # use syntax which works with either 2.x or 3.x
        sys.stderr.write('%s\n' % sys.exc_info()[1])
    sys.exit(rc)
