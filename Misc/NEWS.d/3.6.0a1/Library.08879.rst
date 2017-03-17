Issue #25177: Fixed problem with the mean of very small and very large
numbers. As a side effect, statistics.mean and statistics.variance should
be significantly faster.