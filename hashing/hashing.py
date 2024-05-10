def hashing(lists, canhash=True):
    if canhash:
        if isinstance(lists, list) or isinstance(lists, set):
            return tuple(lists)
        elif isinstance(lists, dict):
            raise ValueError("can't turn dict to tuple")
        else:
            raise ValueError("value is already can hash")
    elif not canhash:
        if isinstance(lists, tuple) or isinstance(lists, frozenset):
            return list(lists)
        else:
            raise ValueError("value is already can't hash")
    else:
        raise ValueError('canhash must be True or False')


l = {'1': 1}
print(hashing(l))
