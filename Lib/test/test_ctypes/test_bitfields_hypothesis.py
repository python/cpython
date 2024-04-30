import dataclasses
import tempfile
from pathlib import Path
import subprocess

import ctypes
from ctypes import (
    Structure,
    alignment,
    sizeof,
    pointer,
    POINTER,
)

from test.support.hypothesis_helper import hypothesis

assume = hypothesis.assume
given = hypothesis.given
strategies = hypothesis.strategies

C_TYPES = {
    # Note: for Hypothesis minimization to work, "simpler" types should
    # generally go first.
    #'char': ctypes.c_char,
    'signed char': ctypes.c_byte,
    'unsigned char': ctypes.c_ubyte,
    'int': ctypes.c_int,
    'unsigned int': ctypes.c_uint,
    **{f'{u}int{n}_t': getattr(ctypes, f'c_{u}int{n}')
      for u in ('', 'u')
      for n in ('8', '16', '32', '64')
    },
    **{f'{"unsigned" if u else "signed"} {t}': getattr(ctypes, f'c_{u}{t}')
      for u in ('', 'u')
      for t in ('short', 'long', 'longlong')
    },
    #'float': ctypes.c_float,
    #'double': ctypes.c_double,
}

def struct_c_decl(attrdict, name='s'):
    lines = []
    pack = attrdict.get('_pack_')
    if pack is not None:
        lines.append(f'#pragma pack({pack})')
    attributes = []
    endian_name = attrdict['endian_name']
    if endian_name != 'native':
        attributes.append(f'scalar_storage_order("{endian_name}-endian")')
    if attrdict.get('_layout_' == 'ms'):
        attributes.append('ms_struct')
        lines.append(f'#pragma ms_struct on')
    lines.append(f'struct')
    if attributes:
        lines.append(f'__attribute__ (( {", ".join(attributes)} ))')
    lines.append(f'     {name} {{')
    for field, ctype_name in zip(attrdict['_fields_'], attrdict['ctype_names']):
        try:
            name, tp, bitsize = field
        except ValueError:
            name, tp = field
            bitfieldspec = ''
        else:
            bitfieldspec = f':{bitsize}'
        lines.append(f'    {ctype_name} {name}{bitfieldspec};')
    lines.append('};')
    lines.append('')
    return '\n'.join(lines)

@strategies.composite
def structure_args(draw):
    num_fields = draw(strategies.integers(1, 50))
    fields = []
    ctype_names = []
    for i in range(num_fields):
        if draw(strategies.booleans()) and 1:
            ctype_name, ctype = draw(strategies.sampled_from(list(C_TYPES.items())))
        else:
            sub_args = draw(structure_args())
            ctype_name = f's{id(sub_args)}'
            ctype = type(sub_args)
        name = f'field_{i}'
        bit_size = draw(strategies.integers(1, sizeof(ctype) * 8))
        is_bitfield = draw(strategies.booleans())
        if is_bitfield:
            fields.append((name, ctype, bit_size))
        else:
            fields.append((name, ctype))
        ctype_names.append(ctype_name)
    endian_name, cls = draw(strategies.sampled_from((
        ('native', ctypes.Structure),
        ('little', ctypes.LittleEndianStructure),
        ('big', ctypes.BigEndianStructure),
    )))

    # XXX: Handle Anonymouses

    attrdict = {
        '_fields_': fields,
        'endian_name': endian_name,
        'ctype_names': ctype_names,
    }
    if draw(strategies.booleans()):
        attrdict['_layout_'] = 'ms'
        pack = 2 ** draw(strategies.integers(0, 4))
        if pack:
            attrdict['_pack_'] = pack
    attrdict['c_decl'] = struct_c_decl(attrdict)
    attrdict['c_name'] = f's{id(attrdict)}'
    result = (
        'RandomTestStruct',
        (cls,),
        attrdict,
    )
    return result


@given(structure_args())
def test_structure(s_args):
    structure = type(*s_args)
    with tempfile.TemporaryDirectory() as tmpdirname:
        temppath = Path(tmpdirname)
        setloop_lines = []
        for field, ctype_name in zip(structure._fields_, structure.ctype_names):
            setloop_lines.append(f'value.{field[0]} = ({ctype_name})-1;')
            setloop_lines.append(f'dump_bytes(&value, sizeof(value));')
        program = (
            r"""
                #include <stdio.h>
                #include <stdalign.h>
                #include <string.h>
                #include <stdint.h>

                #define longlong long long

                %STRUCTDEF%

                void dump_bytes(void *ptr, size_t sz) {
                    unsigned char *cptr = ptr;
                    for (size_t i=0; i<sz; i++) {
                        printf("%02x", (int)cptr[i]);
                    }
                    printf("\n");
                }

                int main(void) {
                    struct s value;
                    printf("%zu\n", sizeof(struct s));
                    printf("%zu\n", alignof(struct s));
                    memset((char*)&value, 0, sizeof(struct s));
                    %SETLOOP%
                    printf("done\n");
                }
            """
            .replace('%STRUCTDEF%', structure.c_decl)
            .replace('%SETLOOP%', '\n'.join(setloop_lines))
        )
        print(program)
        with open(temppath / 'source.c', 'w') as f:
            f.write(program)
        subprocess.run(
            [
                'gcc',
                '-Wno-overflow',
                '-Wno-attributes',
                '-Wno-scalar-storage-order',
                temppath / 'source.c',
                '-o', temppath / 'check_program',
            ],
            check=True,
        )
        proc = subprocess.run(
            [temppath / 'check_program'],
            check=True,
            stdout=subprocess.PIPE,
            encoding='utf-8',
        )
    lines = iter(proc.stdout.splitlines())
    assert int(next(lines)) == sizeof(structure)
    assert int(next(lines)) == alignment(structure)
    print(proc.stdout)
    value = structure()
    for field in value._fields_:
        assert getattr(value, field[0]) == 0
    for field in value._fields_:
        setattr(value, field[0], -1)
        pval = ctypes.string_at(pointer(value), sizeof(structure))
        assert pval.hex() == next(lines)
