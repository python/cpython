import test_support
import random

# Ensure that the seed() method initializes all the hidden state.  In
# particular, through 2.2.1 it failed to reset a piece of state used by
# (and only by) the .gauss() method.

for seed in 1, 12, 123, 1234, 12345, 123456, 654321:
    for seeder in random.seed, random.whseed:
        seeder(seed)
        x1 = random.random()
        y1 = random.gauss(0, 1)

        seeder(seed)
        x2 = random.random()
        y2 = random.gauss(0, 1)

        test_support.vereq(x1, x2)
        test_support.vereq(y1, y2)
