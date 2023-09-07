# Implementation of marshal.loads() in pure Python

import ast

from typing import Any, Tuple


class Type:
    # Adapted from marshal.c
    NULL                = ord('0')
    NONE                = ord('N')
    FALSE               = ord('F')
    TRUE                = ord('T')
    STOPITER            = ord('S')
    ELLIPSIS            = ord('.')
    INT                 = ord('i')
    INT64               = ord('I')
    FLOAT               = ord('f')
    BINARY_FLOAT        = ord('g')
    COMPLEX             = ord('x')
    BINARY_COMPLEX      = ord('y')
    LONG                = ord('l')
    STRING              = ord('s')
    INTERNED            = ord('t')
    REF                 = ord('r')
    TUPLE               = ord('(')
    LIST                = ord('[')
    DICT                = ord('{')
    CODE                = ord('c')
    UNICODE             = ord('u')
    UNKNOWN             = ord('?')
    SET                 = ord('<')
    FROZENSET           = ord('>')
    ASCII               = ord('a')
    ASCII_INTERNED      = ord('A')
    SMALL_TUPLE         = ord(')')
    SHORT_ASCII         = ord('z')
    SHORT_ASCII_INTERNED = ord('Z')


FLAG_REF = 0x80  # with a type, add obj to index

NULL = object()  # marker

# Cell kinds
CO_FAST_LOCAL = 0x20
CO_FAST_CELL = 0x40
CO_FAST_FREE = 0x80


class Code:
    def __init__(self, **kwds: Any):
        self.__dict__.update(kwds)

    def __repr__(self) -> str:
        return f"Code(**{self.__dict__})"

    co_localsplusnames: Tuple[str]
    co_localspluskinds: Tuple[int]

    def get_localsplus_names(self, select_kind: int) -> Tuple[str, ...]:
        varnames: list[str] = []
        for name, kind in zip(self.co_localsplusnames,
                              self.co_localspluskinds):
            if kind & select_kind:
                varnames.append(name)
        return tuple(varnames)

    @property
    def co_varnames(self) -> Tuple[str, ...]:
        return self.get_localsplus_names(CO_FAST_LOCAL)

    @property
    def co_cellvars(self) -> Tuple[str, ...]:
        return self.get_localsplus_names(CO_FAST_CELL)

    @property
    def co_freevars(self) -> Tuple[str, ...]:
        return self.get_localsplus_names(CO_FAST_FREE)

    @property
    def co_nlocals(self) -> int:
        return len(self.co_varnames)


class Reader:
    # A fairly literal translation of the marshal reader.

    def __init__(self, data: bytes):
        self.data: bytes = data
        self.end: int = len(self.data)
        self.pos: int = 0
        self.refs: list[Any] = []
        self.level: int = 0

    def r_string(self, n: int) -> bytes:
        assert 0 <= n <= self.end - self.pos
        buf = self.data[self.pos : self.pos + n]
        self.pos += n
        return buf

    def r_byte(self) -> int:
        buf = self.r_string(1)
        return buf[0]

    def r_short(self) -> int:
        buf = self.r_string(2)
        x = buf[0]
        x |= buf[1] << 8
        x |= -(x & (1<<15))  # Sign-extend
        return x

    def r_long(self) -> int:
        buf = self.r_string(4)
        x = buf[0]
        x |= buf[1] << 8
        x |= buf[2] << 16
        x |= buf[3] << 24
        x |= -(x & (1<<31))  # Sign-extend
        return x

    def r_long64(self) -> int:
        buf = self.r_string(8)
        x = buf[0]
        x |= buf[1] << 8
        x |= buf[2] << 16
        x |= buf[3] << 24
        x |= buf[4] << 32
        x |= buf[5] << 40
        x |= buf[6] << 48
        x |= buf[7] << 56
        x |= -(x & (1<<63))  # Sign-extend
        return x

    def r_PyLong(self) -> int:
        n = self.r_long()
        size = abs(n)
        x = 0
        # Pray this is right
        for i in range(size):
            x |= self.r_short() << i*15
        if n < 0:
            x = -x
        return x

    def r_float_bin(self) -> float:
        buf = self.r_string(8)
        import struct  # Lazy import to avoid breaking UNIX build
        return struct.unpack("d", buf)[0]

    def r_float_str(self) -> float:
        n = self.r_byte()
        buf = self.r_string(n)
        return ast.literal_eval(buf.decode("ascii"))

    def r_ref_reserve(self, flag: int) -> int:
        if flag:
            idx = len(self.refs)
            self.refs.append(None)
            return idx
        else:
            return 0

    def r_ref_insert(self, obj: Any, idx: int, flag: int) -> Any:
        if flag:
            self.refs[idx] = obj
        return obj

    def r_ref(self, obj: Any, flag: int) -> Any:
        assert flag & FLAG_REF
        self.refs.append(obj)
        return obj

    def r_object(self) -> Any:
        old_level = self.level
        try:
            return self._r_object()
        finally:
            self.level = old_level

    def _r_object(self) -> Any:
        code = self.r_byte()
        flag = code & FLAG_REF
        type = code & ~FLAG_REF
        # print("  "*self.level + f"{code} {flag} {type} {chr(type)!r}")
        self.level += 1

        def R_REF(obj: Any) -> Any:
            if flag:
                obj = self.r_ref(obj, flag)
            return obj

        if type == Type.NULL:
            return NULL
        elif type == Type.NONE:
            return None
        elif type == Type.ELLIPSIS:
            return Ellipsis
        elif type == Type.FALSE:
            return False
        elif type == Type.TRUE:
            return True
        elif type == Type.INT:
            return R_REF(self.r_long())
        elif type == Type.INT64:
            return R_REF(self.r_long64())
        elif type == Type.LONG:
            return R_REF(self.r_PyLong())
        elif type == Type.FLOAT:
            return R_REF(self.r_float_str())
        elif type == Type.BINARY_FLOAT:
            return R_REF(self.r_float_bin())
        elif type == Type.COMPLEX:
            return R_REF(complex(self.r_float_str(),
                                    self.r_float_str()))
        elif type == Type.BINARY_COMPLEX:
            return R_REF(complex(self.r_float_bin(),
                                    self.r_float_bin()))
        elif type == Type.STRING:
            n = self.r_long()
            return R_REF(self.r_string(n))
        elif type == Type.ASCII_INTERNED or type == Type.ASCII:
            n = self.r_long()
            return R_REF(self.r_string(n).decode("ascii"))
        elif type == Type.SHORT_ASCII_INTERNED or type == Type.SHORT_ASCII:
            n = self.r_byte()
            return R_REF(self.r_string(n).decode("ascii"))
        elif type == Type.INTERNED or type == Type.UNICODE:
            n = self.r_long()
            return R_REF(self.r_string(n).decode("utf8", "surrogatepass"))
        elif type == Type.SMALL_TUPLE:
            n = self.r_byte()
            idx = self.r_ref_reserve(flag)
            retval: Any = tuple(self.r_object() for _ in range(n))
            self.r_ref_insert(retval, idx, flag)
            return retval
        elif type == Type.TUPLE:
            n = self.r_long()
            idx = self.r_ref_reserve(flag)
            retval = tuple(self.r_object() for _ in range(n))
            self.r_ref_insert(retval, idx, flag)
            return retval
        elif type == Type.LIST:
            n = self.r_long()
            retval = R_REF([])
            for _ in range(n):
                retval.append(self.r_object())
            return retval
        elif type == Type.DICT:
            retval = R_REF({})
            while True:
                key = self.r_object()
                if key == NULL:
                    break
                val = self.r_object()
                retval[key] = val
            return retval
        elif type == Type.SET:
            n = self.r_long()
            retval = R_REF(set())
            for _ in range(n):
                v = self.r_object()
                retval.add(v)
            return retval
        elif type == Type.FROZENSET:
            n = self.r_long()
            s: set[Any] = set()
            idx = self.r_ref_reserve(flag)
            for _ in range(n):
                v = self.r_object()
                s.add(v)
            retval = frozenset(s)
            self.r_ref_insert(retval, idx, flag)
            return retval
        elif type == Type.CODE:
            retval = R_REF(Code())
            retval.co_argcount = self.r_long()
            retval.co_posonlyargcount = self.r_long()
            retval.co_kwonlyargcount = self.r_long()
            retval.co_stacksize = self.r_long()
            retval.co_flags = self.r_long()
            retval.co_code = self.r_object()
            retval.co_consts = self.r_object()
            retval.co_names = self.r_object()
            retval.co_localsplusnames = self.r_object()
            retval.co_localspluskinds = self.r_object()
            retval.co_filename = self.r_object()
            retval.co_name = self.r_object()
            retval.co_qualname = self.r_object()
            retval.co_firstlineno = self.r_long()
            retval.co_linetable = self.r_object()
            retval.co_exceptiontable = self.r_object()
            return retval
        elif type == Type.REF:
            n = self.r_long()
            retval = self.refs[n]
            assert retval is not None
            return retval
        else:
            breakpoint()
            raise AssertionError(f"Unknown type {type} {chr(type)!r}")


def loads(data: bytes) -> Any:
    assert isinstance(data, bytes)
    r = Reader(data)
    return r.r_object()


def main():
    # Test
    import marshal, pprint
    sample = {'foo': {(42, "bar", 3.14)}}
    data = marshal.dumps(sample)
    retval = loads(data)
    assert retval == sample, retval
    sample = main.__code__
    data = marshal.dumps(sample)
    retval = loads(data)
    assert isinstance(retval, Code), retval
    pprint.pprint(retval.__dict__)


if __name__ == "__main__":
    main()
