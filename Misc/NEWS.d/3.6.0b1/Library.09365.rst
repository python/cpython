Issue #27706: Restore deterministic behavior of random.Random().seed()
for string seeds using seeding version 1.  Allows sequences of calls
to random() to exactly match those obtained in Python 2.
Patch by Nofar Schnider.