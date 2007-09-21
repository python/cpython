#
# this is a rather strict implementation of a bit vector class
# it is accessed the same way as an array of python-ints, except
# the value must be 0 or 1
#

import sys; rprt = sys.stderr.write #for debugging

error = 'bitvec.error'


def _check_value(value):
    if type(value) != type(0) or not 0 <= value < 2:
        raise error('bitvec() items must have int value 0 or 1')


import math

def _compute_len(param):
    mant, l = math.frexp(float(param))
    bitmask = 1 << l
    if bitmask <= param:
        raise ValueError('(param, l) = %r' % ((param, l),))
    while l:
        bitmask = bitmask >> 1
        if param & bitmask:
            break
        l = l - 1
    return l


def _check_key(len, key):
    if type(key) != type(0):
        raise TypeError('sequence subscript not int')
    if key < 0:
        key = key + len
    if not 0 <= key < len:
        raise IndexError('list index out of range')
    return key

def _check_slice(len, i, j):
    #the type is ok, Python already checked that
    i, j = max(i, 0), min(len, j)
    if i > j:
        i = j
    return i, j


class BitVec:

    def __init__(self, *params):
        self._data = 0
        self._len = 0
        if not len(params):
            pass
        elif len(params) == 1:
            param, = params
            if type(param) == type([]):
                value = 0
                bit_mask = 1
                for item in param:
                    # strict check
                    #_check_value(item)
                    if item:
                        value = value | bit_mask
                    bit_mask = bit_mask << 1
                self._data = value
                self._len = len(param)
            elif type(param) == type(0):
                if param < 0:
                    raise error('bitvec() can\'t handle negative longs')
                self._data = param
                self._len = _compute_len(param)
            else:
                raise error('bitvec() requires array or long parameter')
        elif len(params) == 2:
            param, length = params
            if type(param) == type(0):
                if param < 0:
                    raise error('can\'t handle negative longs')
                self._data = param
                if type(length) != type(0):
                    raise error('bitvec()\'s 2nd parameter must be int')
                computed_length = _compute_len(param)
                if computed_length > length:
                    print('warning: bitvec() value is longer than the length indicates, truncating value')
                    self._data = self._data & \
                              ((1 << length) - 1)
                self._len = length
            else:
                raise error('bitvec() requires array or long parameter')
        else:
            raise error('bitvec() requires 0 -- 2 parameter(s)')


    def append(self, item):
        #_check_value(item)
        #self[self._len:self._len] = [item]
        self[self._len:self._len] = \
                  BitVec(int(not not item), 1)


    def count(self, value):
        #_check_value(value)
        if value:
            data = self._data
        else:
            data = (~self)._data
        count = 0
        while data:
            data, count = data >> 1, count + (data & 1 != 0)
        return count


    def index(self, value):
        #_check_value(value):
        if value:
            data = self._data
        else:
            data = (~self)._data
        index = 0
        if not data:
            raise ValueError('list.index(x): x not in list')
        while not (data & 1):
            data, index = data >> 1, index + 1
        return index


    def insert(self, index, item):
        #_check_value(item)
        #self[index:index] = [item]
        self[index:index] = BitVec(int(not not item), 1)


    def remove(self, value):
        del self[self.index(value)]


    def reverse(self):
        #ouch, this one is expensive!
        #for i in self._len>>1: self[i], self[l-i] = self[l-i], self[i]
        data, result = self._data, 0
        for i in range(self._len):
            if not data:
                result = result << (self._len - i)
                break
            result, data = (result << 1) | (data & 1), data >> 1
        self._data = result


    def sort(self):
        c = self.count(1)
        self._data = ((1 << c) - 1) << (self._len - c)


    def copy(self):
        return BitVec(self._data, self._len)


    def seq(self):
        result = []
        for i in self:
            result.append(i)
        return result


    def __repr__(self):
        ##rprt('<bitvec class instance object>.' + '__repr__()\n')
        return 'bitvec(%r, %r)' % (self._data, self._len)

    def __cmp__(self, other, *rest):
        #rprt('%r.__cmp__%r\n' % (self, (other,) + rest))
        if type(other) != type(self):
            other = bitvec(other, *rest)
        #expensive solution... recursive binary, with slicing
        length = self._len
        if length == 0 or other._len == 0:
            return cmp(length, other._len)
        if length != other._len:
            min_length = min(length, other._len)
            return cmp(self[:min_length], other[:min_length]) or \
                      cmp(self[min_length:], other[min_length:])
        #the lengths are the same now...
        if self._data == other._data:
            return 0
        if length == 1:
            return cmp(self[0], other[0])
        else:
            length = length >> 1
            return cmp(self[:length], other[:length]) or \
                      cmp(self[length:], other[length:])


    def __len__(self):
        #rprt('%r.__len__()\n' % (self,))
        return self._len

    def __getitem__(self, key):
        #rprt('%r.__getitem__(%r)\n' % (self, key))
        key = _check_key(self._len, key)
        return self._data & (1 << key) != 0

    def __setitem__(self, key, value):
        #rprt('%r.__setitem__(%r, %r)\n' % (self, key, value))
        key = _check_key(self._len, key)
        #_check_value(value)
        if value:
            self._data = self._data | (1 << key)
        else:
            self._data = self._data & ~(1 << key)

    def __delitem__(self, key):
        #rprt('%r.__delitem__(%r)\n' % (self, key))
        key = _check_key(self._len, key)
        #el cheapo solution...
        self._data = self[:key]._data | self[key+1:]._data >> key
        self._len = self._len - 1

    def __getslice__(self, i, j):
        #rprt('%r.__getslice__(%r, %r)\n' % (self, i, j))
        i, j = _check_slice(self._len, i, j)
        if i >= j:
            return BitVec(0, 0)
        if i:
            ndata = self._data >> i
        else:
            ndata = self._data
        nlength = j - i
        if j != self._len:
            #we'll have to invent faster variants here
            #e.g. mod_2exp
            ndata = ndata & ((1 << nlength) - 1)
        return BitVec(ndata, nlength)

    def __setslice__(self, i, j, sequence, *rest):
        #rprt('%s.__setslice__%r\n' % (self, (i, j, sequence) + rest))
        i, j = _check_slice(self._len, i, j)
        if type(sequence) != type(self):
            sequence = bitvec(sequence, *rest)
        #sequence is now of our own type
        ls_part = self[:i]
        ms_part = self[j:]
        self._data = ls_part._data | \
                  ((sequence._data | \
                  (ms_part._data << sequence._len)) << ls_part._len)
        self._len = self._len - j + i + sequence._len

    def __delslice__(self, i, j):
        #rprt('%r.__delslice__(%r, %r)\n' % (self, i, j))
        i, j = _check_slice(self._len, i, j)
        if i == 0 and j == self._len:
            self._data, self._len = 0, 0
        elif i < j:
            self._data = self[:i]._data | (self[j:]._data >> i)
            self._len = self._len - j + i

    def __add__(self, other):
        #rprt('%r.__add__(%r)\n' % (self, other))
        retval = self.copy()
        retval[self._len:self._len] = other
        return retval

    def __mul__(self, multiplier):
        #rprt('%r.__mul__(%r)\n' % (self, multiplier))
        if type(multiplier) != type(0):
            raise TypeError('sequence subscript not int')
        if multiplier <= 0:
            return BitVec(0, 0)
        elif multiplier == 1:
            return self.copy()
        #handle special cases all 0 or all 1...
        if self._data == 0:
            return BitVec(0, self._len * multiplier)
        elif (~self)._data == 0:
            return ~BitVec(0, self._len * multiplier)
        #otherwise el cheapo again...
        retval = BitVec(0, 0)
        while multiplier:
            retval, multiplier = retval + self, multiplier - 1
        return retval

    def __and__(self, otherseq, *rest):
        #rprt('%r.__and__%r\n' % (self, (otherseq,) + rest))
        if type(otherseq) != type(self):
            otherseq = bitvec(otherseq, *rest)
        #sequence is now of our own type
        return BitVec(self._data & otherseq._data, \
                  min(self._len, otherseq._len))


    def __xor__(self, otherseq, *rest):
        #rprt('%r.__xor__%r\n' % (self, (otherseq,) + rest))
        if type(otherseq) != type(self):
            otherseq = bitvec(otherseq, *rest)
        #sequence is now of our own type
        return BitVec(self._data ^ otherseq._data, \
                  max(self._len, otherseq._len))


    def __or__(self, otherseq, *rest):
        #rprt('%r.__or__%r\n' % (self, (otherseq,) + rest))
        if type(otherseq) != type(self):
            otherseq = bitvec(otherseq, *rest)
        #sequence is now of our own type
        return BitVec(self._data | otherseq._data, \
                  max(self._len, otherseq._len))


    def __invert__(self):
        #rprt('%r.__invert__()\n' % (self,))
        return BitVec(~self._data & ((1 << self._len) - 1), \
                  self._len)

    def __int__(self):
        return int(self._data)

    def __long__(self):
        return int(self._data)

    def __float__(self):
        return float(self._data)


bitvec = BitVec
