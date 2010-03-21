class Security:

    def __init__(self):
        import os
        env = os.environ
        if 'PYTHON_KEYFILE' in env:
            keyfile = env['PYTHON_KEYFILE']
        else:
            keyfile = '.python_keyfile'
            if 'HOME' in env:
                keyfile = os.path.join(env['HOME'], keyfile)
            if not os.path.exists(keyfile):
                import sys
                for dir in sys.path:
                    kf = os.path.join(dir, keyfile)
                    if os.path.exists(kf):
                        keyfile = kf
                        break
        try:
            self._key = eval(open(keyfile).readline())
        except IOError:
            raise IOError("python keyfile %s: cannot open" % keyfile)

    def _generate_challenge(self):
        import random
        return random.randint(100, 100000)

    def _compare_challenge_response(self, challenge, response):
        return self._encode_challenge(challenge) == response

    def _encode_challenge(self, challenge):
        p, m = self._key
        return pow(int(challenge), p, m)
