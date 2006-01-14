
# http://python.org/sf/1202533

if __name__ == '__main__':
    lst = [apply]
    lst.append(lst)
    apply(*lst)      # segfault: infinite recursion in C
