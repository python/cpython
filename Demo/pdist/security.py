class Security:

	def __init__(self):
		import os
		env = os.environ
		if env.has_key('PYTHON_KEYFILE'):
			keyfile = env['PYTHON_KEYFILE']
		elif env.has_key('HOME'):
			keyfile = env['HOME'] + '.python_keyfile'
		else:
			keyfile = '.python_keyfile'
		try:
			self._key = eval(open(keyfile).readline())
		except IOError:
			raise IOError, "python keyfile %s not found" % keyfile

	def _generate_challenge(self):
		import whrandom
		return whrandom.randint(100, 100000)

	def _compare_challenge_response(self, challenge, response):
		return self._encode_challenge(challenge) == response

	def _encode_challenge(self, challenge):
		p, m = self._key
		return pow(challenge, p, m)
