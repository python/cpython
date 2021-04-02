import opcode
import sys


ADD_INT = opcode.opmap['ADD_INT']


def all_code_objects(code):
    yield code
    for x in code.co_consts:
        if hasattr(x, 'co_code'):
            yield x


def report(code):
    add_int_count = total_count = 0
    for co in all_code_objects(code):
        co_code = co.co_code
        for i in range(0, len(co_code), 2):
            op = co_code[i]
            if op == ADD_INT:
                add_int_count += 1
            else:
                total_count += 1
    print(add_int_count, "/", total_count,
          f"{add_int_count/total_count*100:.2f}%")
    return add_int_count, total_count


def main(dirname):
    import os
    add_int_count = total_count = 0
    for root, dirs, files in os.walk(dirname):
        for file in files:
            if file.endswith(".py"):
                full = os.path.join(root, file)
                try:
                    with open(full, "r") as f:
                        source = f.read()
                    code = compile(source, full, "exec")
                except Exception as err:
                    print(full + ":", err)
                    continue
                print(full, end=" ")
                a, b = report(code)
                add_int_count += a
                total_count += b
    print("TOTAL", add_int_count, "/", total_count,
          f"{add_int_count/total_count*100:.2f}%")


if __name__ == "__main__":
    main(sys.argv[1])
