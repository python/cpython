# Module 'rand'

import whrandom

def srand(seed):
	whrandom.seed(seed%256, seed/256%256, seed/65536%256)

def rand():
	return int(whrandom.random() * 32768.0) % 32768

def choice(seq):
	return seq[rand() % len(seq)]
