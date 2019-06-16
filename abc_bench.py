import csv
import timeit


def median(x):
    n = len(x)
    middle = n // 2
    sx = sorted(x)
    if n % 2:
        return sx[middle]
    else:
        return (sx[middle] + sx[middle - 1]) / 2


implementations = {"C": "import abc",
                   "Py": "import _py_abc"}
statements = {
    "C": {
        "master": "abc.ABCMeta('ABC_C', (), {'__module__': __name__})",
        "fix": "abc.ABCMeta('ABC_C', (), {})"
    },
    "Py": {
        "master": "_py_abc.ABCMeta('ABC_Py', (), {'__module__': __name__})",
        "fix": "_py_abc.ABCMeta('ABC_Py', (), {})"
    }
}

repeat = 50000
number = 1

data = {}
for imp, setup in implementations.items():
    for branch, stmt in statements[imp].items():
        print("timing {} - {} implementation of ABCMeta...".format(branch, imp))
        times = timeit.repeat(stmt, setup=setup, repeat=repeat, number=number)
        header = "{}_{}".format(branch, imp)
        data[header] = times


for imp in implementations:
    t_master = median(data["master_{}".format(imp)])
    t_fix = median(data["fix_{}".format(imp)])

    absdiff = t_fix - t_master
    slowdown = (t_fix - t_master) / t_master

    print("{} implementation".format(imp))
    print("    Absolute difference:", 1000 * absdiff, "ms")
    print("    Slowdown:", 100 * slowdown, "%")

print("Dumping to CSV file...")
with open("abc_bench.csv", "w") as csvfile:
    writer = csv.writer(csvfile)
    data = [[header] + times for header, times in data.items()]
    for row in zip(*data):
        writer.writerow(row)
