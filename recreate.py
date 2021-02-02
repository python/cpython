# Todo

# when a call expression is spread over multiple lines, step into fails
# should step forward when it pops a stack, come back to the beginning of the state of the line in parent scope,
#  or after?
# ability to filter tracing only to the code you choose
# get classes to work as objects
# log exceptions
# get pygame working
# get ascii draw working
# get flask working
# get caves (xlrd) working
# compile SSL into Python
# collect "real world" python apps
# write terminal debugger in python
# have a flag to turn on debug mode
# try it on a "real" app
# optimization: use low level iteration methods whenever possible

# should we make stack variables heap objects the same way Python does it? (done)
#    So that del var statements can work naturally (done)
# global vars in a module (done)
# delete global (done)
# nonlocal set variable (done)
# optimization: perf of recreate.py (probably transactions) (done)
# support for slices (ascii_draw.py) (done)
# make log file same prefix name as program name (done)
# global variables not showing for convert_points.py (done)
# when last line in a program is a function call, you don't get to step to the end of the program (shape_calculator.py) (done)
# clean up server.js code (done)
# bug in rpg game __doc__ = --------------- (done)
# bug in rpg game where step over but actually step into (done)
# make modules work (done)
# get rpg game working (done)
# make work for multiple files (done)
# make exceptions work (done)
# make work for generator expressions (done)
# put rewind.o in modules permanently (don't edit Makefile) (done)
# objects (done)
# sets (done)
# * set.add (done)
# * set.discard (done)
# * set.remove (done)
# * set.update
# * set.clear (done)
# * -= (done)
# * &= (done)
# * ^= (done)
# * |= (done)
# * difference_update (done)
# * intersection_update (done)
# * symmetric_difference_update (done)
# bug: entries wo line no (done)
# clean up reclaimed objects (done)
# dicts (done)
# * dict.update (done)
# * dict.clear (done)
# * dict.pop (done)
# * dict.popitem (done)
# make line parsing generic (done)
# dealing with dict key and set uniqueness (done)
# list sort (done)
# list pop index parameter (done)
# tuples (done)
# lists
#   * append(done)
#   * extend (done)
#   * delete subscript (done)
#   * store subscript (done)
#   * insert (done)
#   * remove (done)
#   * pop (done)
#   * clear (done)
#   * reverse (done)
#   * list within lists (heap object refs)
# switch store subscript to intercepting data structures at low level (done)
# should we remove intercept for BUILD_LIST (done)
# bug: shortest_common_supersequence.py strings look like {} (done)
# get log output to trim the initialization (done)
# strings (done)
# bug: permutations_2.py rest looks like {}? (done)
# test debugger on basic example (done)
# change ID scheme for class-based objects (done)
# Trim initialization code execution (done)
# STORE_NAME (done)
# Track return value of functions (done)
# Solve the object ID reused problem (done)
# Store Fast (done)

import sqlite3
import os
import os.path
import re
import sys

def define_schema(conn):
    c = conn.cursor()

    c.execute("""
        create table Snapshot (
            id integer primary key,
            fun_call_id integer,
            heap integer,
            line_no integer,
            error_id integer,
            constraint Snapshot_fk_fun_call_id foreign key (fun_call_id)
                references FunCall(id)
            constraint Snapshot_fk_error_id foreign key (error_id)
                references Error(id)
        );
    """)

    c.execute("""
        create table Object (
            id integer primary key,
            data text  -- JSONR format
        );
    """)

    c.execute("""
        create table FunCall (
            id integer primary key,
            fun_name text,
            locals integer, -- heap ID
            globals integer, -- heap ID
            closure_cellvars text, -- json-like object
            closure_freevars text, -- json-like object
            parent_id integer,
            code_file_id integer,
            
            constraint FunCall_fk_parent_id foreign key (parent_id)
                references FunCall(id)
            constraint FunCall_fk_code_file_id foreign key (code_file_id)
                references CodeFile(id)
        );
    """)

    c.execute(""" 
        create table CodeFile (
            id integer primary key,
            file_path text,
            source text
        );
    """)

    c.execute("""
        create table HeapRef (
            id integer,
            heap_version integer,
            object_id integer,

            constraint HeapRef_pk PRIMARY KEY (id, heap_version)
            constraint HeapRef_fk_object_id foreign key (object_id)
                references Object(id)
        );
    """)

    c.execute("""
        create table Error (
            id integer primary key,
            message text
        );
    """)

    conn.commit()

NUMBER_REGEX = r"-?(?:(?:[1-9][0-9]*)|[0-9])(?:\.[0-9]+)?"
CHAR_REGEX = r"[^'\\]|(?:\\(?:['\\ntr]|x[0-9A-Fa-f]{2}))"
CHAR2_REGEX = r"[^\"\\]|(?:\\(?:[\"\\ntr]|x[0-9A-Fa-f]{2}))"
STRING_REGEX = r"(?:'(?:%s)*')|(?:\"(?:%s)*\")" % (CHAR_REGEX, CHAR2_REGEX)
BOOLEAN_REGEX = r"True|False"
REF_REGEX = r"\*[0-9]+"
NONE_REGEX = r"None"
ARGUMENT_REGEX = re.compile("(%s|%s|%s|%s|%s)[,)]" % (
    NUMBER_REGEX,
    STRING_REGEX,
    BOOLEAN_REGEX,
    REF_REGEX,
    NONE_REGEX
))
LINE_START_REGEX = re.compile(r"([A-Z_]+)\(")

def parse_value(value):
    if value[0] == "*":
        return HeapRef(int(value[1:]))
    else:
        return eval(value)

def parse_line(line):
    args = []
    m = LINE_START_REGEX.match(line)
    fun_name = m.group(1)
    start_idx = m.span()[1]
    while True:
        m = ARGUMENT_REGEX.match(line, start_idx)
        if m is None:
            raise Exception("Parse error on line '%s' at %d" % (line, start_idx))
        value = parse_value(m.group(1))
        args.append(value)
        if m.span()[1] == len(line):
            break
        if line[m.span()[1]] == "\n":
            break 
        assert line[m.span()[1]] == " "
        start_idx = m.span()[1] + 1
    return (fun_name, args)

class ObjectRef(object):
    def __init__(self, id):
        self.id = id

    def serialize_member(self):
        return '*' + str(self.id)

    def __repr__(self):
        return f"ObjectRef({self.id})"       

class HeapRef(object):
    def __init__(self, id):
        self.id = id

    def serialize_member(self):
        return '^' + str(self.id)

    def __repr__(self):
        return f"HeapRef({self.id})"
    
    def __eq__(self, other):
        return self.id == other.id

    def __hash__(self):
        return self.id


def recreate_past(conn, filename):

    fun_lookup = {}
    
    class FunCall(object):
        def __init__(self, 
            id, name, 
            local_varnames, 
            local_vars_id,
            global_vars_id, 
            cell_vars,
            free_vars,
            parent,
            code_file_id):
            self.id = id
            self.name = name
            self.local_varnames = local_varnames
            self.local_vars_id = local_vars_id
            self.global_vars_id = global_vars_id
            self.cell_vars = cell_vars
            self.free_vars = free_vars
            self.parent = parent
            self.code_file_id = code_file_id

        def save(self, cursor):
            cursor.execute("INSERT INTO FunCall VALUES (?, ?, ?, ?, ?, ?, ?, ?)", (
                self.id,
                self.name,
                self.local_vars_id,
                self.global_vars_id,
                serialize(self.cell_vars),
                serialize(self.free_vars),
                self.parent and self.parent.id,
                self.code_file_id
            ))

        def __repr__(self):
            return "<" + str(self.id) + ">" + self.name + "(" + str(self.varnames) + ")"

    def quote(value):
        return '"' + value.replace('"', '\\"').replace('\\', '\\\\') + '"'

    def new_obj_id():
        nonlocal next_object_id
        id = next_object_id
        next_object_id += 1
        return id

    def new_snapshot_id():
        nonlocal next_snapshot_id
        snapshot_id = next_snapshot_id
        next_snapshot_id += 1
        return snapshot_id

    def new_fun_call_id():
        nonlocal next_fun_call_id
        fun_call_id = next_fun_call_id
        next_fun_call_id += 1
        return fun_call_id
    
    def immut_id(obj):
        nonlocal object_id_to_immutable_id_dict
        if id(obj) in object_id_to_immutable_id_dict:
            return object_id_to_immutable_id_dict[id(obj)]
        else:
            raise Exception("No entry for object ID " + str(id(obj)) + ": " + repr(obj))

    def serialize_member(value):
        if hasattr(value, "serialize_member"):
            return value.serialize_member()
        elif value is None:
            return "null"
        elif isinstance(value, bool):
            if value:
                return "true"
            else:
                return "false"
        elif isinstance(value, int):
            return str(value)
        elif isinstance(value, float):
            return str(value)
        elif isinstance(value, str):
            return quote(value)
        else:
            oid = immut_id(value)
            return "*" + str(oid)

    def format_key_value_pair(item):
        # TODO: update jsonr parser syntax to support refs for keys
        # return quote(str(item[0])) + ": " + serialize_member(item[1])
        value = serialize_member(item[1])
        key = serialize_member(item[0])
        return key + ": " + value

    def serialize(value):
        if hasattr(value, "serialize"):
            return value.serialize()
        if isinstance(value, list):
            return "[" + ", ".join(map(serialize_member, value)) + "]"
        elif isinstance(value, dict):
            return "{" + ", ".join(map(format_key_value_pair, value.items())) + "}"
        elif isinstance(value, tuple):
            return "[" + ", ".join(map(serialize_member, value)) + "]"
        elif isinstance(value, set):
            return "[" + ", ".join(map(serialize_member, value)) + "]"
        else:
            return serialize_member(value)

    def save_object(obj):
        nonlocal object_id_to_immutable_id_dict
        oid = None
        if obj == []:
            oid = empty_list_oid
        if obj == {}:
            oid =  empty_dict_oid
        if obj == empty_set:
            oid =  empty_set_oid

        if oid is None:
            oid = new_obj_id()
            object_id_to_immutable_id_dict[id(obj)] = oid
            if isinstance(obj, dict) and "global_test_module" in obj:
                raise "here"
            cursor.execute("INSERT INTO Object VALUES (?, ?)", (
                oid,
                serialize(obj)
            ))
        else:
            object_id_to_immutable_id_dict[id(obj)] = oid

        return oid

    def _save_object(obj, oid):
        nonlocal object_id_to_immutable_id_dict
        
        object_id_to_immutable_id_dict[id(obj)] = oid
        cursor.execute("INSERT INTO Object VALUES (?, ?)", (
            oid,
            serialize(obj)
        ))
        return oid
    
    def update_heap_object(heap_id, new_obj):
        nonlocal heap_version
        nonlocal heap_id_to_object_dict

        heap_version += 1

        heap_id_to_object_dict[heap_id] = new_obj
        oid = save_object(new_obj)

        cursor.execute("INSERT INTO HeapRef VALUES (?, ?, ?)", (
            heap_id,
            heap_version,
            oid
        ))

    def ensure_code_file_saved(filename, name):
        nonlocal activate_snapshots
        nonlocal next_code_file_id

        if filename not in code_files and os.path.isfile(filename):
            if not activate_snapshots and name == "<module>":
                activate_snapshots = True
            the_file = open(filename, "r")
            content = the_file.read()
            the_file.close()
            code_file_id = next_code_file_id
            next_code_file_id += 1
            cursor.execute("INSERT INTO CodeFile VALUES (?, ?, ?)", (
                code_file_id,
                filename,
                content
            ))
            code_files[filename] = code_file_id

    def grab_var_args(a_list):
        num_args = a_list[0]
        args = a_list[1:1 + num_args]
        rest = a_list[1 + num_args:]
        return num_args, args, rest

    def gen_var_dict(varnames, values):
        var_dict = {}
        for i in range(len(varnames)):
            varname = varnames[i]
            value = values[i]
            var_dict[varname] = value
        return var_dict

    def process_push_frame(filename, name, global_vars_id, local_vars_id, *rest):
        nonlocal stack
        
        num_local_vars, local_varnames, rest = grab_var_args(rest)
        num_cell_vars, cell_varnames, rest = grab_var_args(rest)
        num_free_vars, free_varnames, rest = grab_var_args(rest)

        local_vars = rest[0:num_local_vars]
        rest = rest[num_local_vars:]
        cell_vars = rest[0:num_cell_vars]
        free_vars = rest[num_cell_vars:]

        local_var_dict = gen_var_dict(local_varnames, local_vars)
        cell_var_dict = gen_var_dict(cell_varnames, cell_vars)
        free_var_dict = gen_var_dict(free_varnames, free_vars)

        #print("push_frame", name, "cell_var_dict", cell_var_dict, "free_var_dict", free_var_dict)

        ensure_code_file_saved(filename, name)
        code_file_id = code_files.get(filename)

        update_heap_object(local_vars_id, local_var_dict)

        fun_call = FunCall(
            new_fun_call_id(),
            name,
            local_varnames,
            local_vars_id,
            global_vars_id,
            cell_var_dict,
            free_var_dict,
            stack,
            code_file_id
        )
        fun_call.save(cursor)

        stack = fun_call
    
    fun_lookup["PUSH_FRAME"] = process_push_frame

    def process_visit(line_no):
        nonlocal curr_line_no
        curr_line_no = line_no
        if not activate_snapshots:
            return
        cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?)", (
            new_snapshot_id(), 
            stack and stack.id, 
            heap_version,
            line_no,
            None
        ))
    
    fun_lookup["VISIT"] = process_visit

    def process_store_name(heap_id, name, value):
        ns = heap_id_to_object_dict[heap_id]
        new_ns = ns.copy()
        new_ns[name] = value
        update_heap_object(heap_id, new_ns)

    fun_lookup["STORE_NAME"] = process_store_name
    fun_lookup["STORE_GLOBAL"] = process_store_name
    
    def process_store_fast(index, value):
        varname = stack.local_varnames[index]
        local_vars = heap_id_to_object_dict[stack.local_vars_id]
        new_local_vars = local_vars.copy()
        new_local_vars[varname] = value
        update_heap_object(stack.local_vars_id, new_local_vars)
    
    fun_lookup["STORE_FAST"] = process_store_fast

    def process_store_deref(heap_id, value):
        cell = heap_id_to_object_dict[heap_id]
        new_cell = cell.copy()
        new_cell["ob_ref"] = value
        update_heap_object(heap_id, new_cell)

    fun_lookup["STORE_DEREF"] = process_store_deref
    
    def process_return_value(ret_val, *extra_params):
        if not activate_snapshots:
            return

        heap_id = stack.local_vars_id
        local_vars = heap_id_to_object_dict[heap_id]
        new_local_vars = local_vars.copy()
        new_local_vars["<ret val>"] = ret_val
        update_heap_object(heap_id, new_local_vars)

        cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?)", (
            new_snapshot_id(), 
            stack and stack.id, 
            heap_version,
            curr_line_no, 
            None
        ))

    fun_lookup["RETURN_VALUE"] = process_return_value
    fun_lookup["YIELD_VALUE"] = process_return_value

    def process_pop_frame(filename, name):
        nonlocal stack
        try:
            assert stack.name == name
            stack = stack.parent
        except:
            print(log_line_no, curr_line_no, line)
            print("expected:", filename, name)
            print("Got:", stack[0].fun_call)
            raise "stack function names not matching"
    
    fun_lookup["POP_FRAME"] = process_pop_frame
    
    def process_new_list(heap_id, *a_list):
        update_heap_object(heap_id, list(a_list))

    fun_lookup["NEW_LIST"] = process_new_list
    
    def process_new_string(heap_id, string):
        update_heap_object(heap_id, string)
    
    fun_lookup["NEW_STRING"] = process_new_string
    
    def process_list_append(heap_id, item):
        a_list = heap_id_to_object_dict[heap_id]
        a_list = a_list + [item]
        update_heap_object(heap_id, a_list)
    
    fun_lookup["LIST_APPEND"] = process_list_append
    
    def process_list_extend(heap_id, other_list_heap_ref):
        a_list = heap_id_to_object_dict[heap_id]
        other_list = heap_id_to_object_dict[other_list_heap_ref.id]
        new_a_list = a_list + list(other_list)
        update_heap_object(heap_id, new_a_list)
    
    fun_lookup["LIST_EXTEND"] = process_list_extend
    
    def process_new_tuple(heap_id, *a_tuple):
        update_heap_object(heap_id, a_tuple)
    
    fun_lookup["NEW_TUPLE"] = process_new_tuple

    def process_list_store_subscript(heap_id, index, value):
        obj = heap_id_to_object_dict[heap_id]
        new_list = obj.copy()
        new_list[index] = value
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_STORE_SUBSCRIPT"] = process_list_store_subscript

    def process_list_store_subscript_slice(heap_id, start, stop, step, value):
        a_list = heap_id_to_object_dict[heap_id]
        new_list = a_list = a_list.copy()
        a_slice = slice(start, stop, step)
        if isinstance(value, HeapRef):
            value = heap_id_to_object_dict[value.id]
        new_list[a_slice] = value
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_STORE_SUBSCRIPT_SLICE"] = process_list_store_subscript_slice

    def process_list_delete_subscript(heap_id, index):
        obj = heap_id_to_object_dict[heap_id]
        new_obj = obj.copy()
        del new_obj[index]
        update_heap_object(heap_id, new_obj)
    
    fun_lookup["LIST_DELETE_SUBSCRIPT"] = process_list_delete_subscript

    def process_list_delete_subscript_slice(heap_id, start, stop, step):
        a_list = heap_id_to_object_dict[heap_id]
        new_list = a_list = a_list.copy()
        a_slice = slice(start, stop, step)
        del new_list[a_slice]
        update_heap_object(heap_id, new_list)

    fun_lookup["LIST_DELETE_SUBSCRIPT_SLICE"] = process_list_delete_subscript_slice

    def process_list_insert(heap_id, index, value):
        obj = heap_id_to_object_dict[heap_id]
        new_list = obj.copy()
        new_list.insert(index, value)
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_INSERT"] = process_list_insert

    def process_list_remove(heap_id, value):
        obj = heap_id_to_object_dict[heap_id]
        new_list = obj.copy()
        new_list.remove(value)
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_REMOVE"] = process_list_remove

    def process_list_pop(heap_id, index):
        a_list = heap_id_to_object_dict[heap_id]
        new_list = a_list.copy()
        new_list.pop(index)
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_POP"] = process_list_pop

    def process_list_clear(heap_id):
        update_heap_object(heap_id, [])
    
    fun_lookup["LIST_CLEAR"] = process_list_clear

    def process_list_reverse(heap_id):
        a_list = heap_id_to_object_dict[heap_id]
        new_list = a_list.copy()
        new_list.reverse()
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_REVERSE"] = process_list_reverse
    
    def process_list_sort(heap_id, *new_list):
        update_heap_object(heap_id, list(new_list))
    
    fun_lookup["LIST_SORT"] = process_list_sort

    def process_string_inplace_add_result(heap_id, string):
        update_heap_object(heap_id, string)

    fun_lookup["STRING_INPLACE_ADD_RESULT"]  = process_string_inplace_add_result
    
    def process_new_dict(heap_id, *entries):
        a_dict = {}
        for i in range(0, len(entries), 2):
            key = entries[i]
            value = entries[i + 1]
            a_dict[key] = value
        update_heap_object(heap_id, a_dict)
    
    fun_lookup["NEW_DICT"] = process_new_dict
    
    def process_dict_store_subscript(heap_id, key, value):
        a_dict = heap_id_to_object_dict[heap_id]
        new_dict = a_dict.copy()
        new_dict[key] = value
        update_heap_object(heap_id, new_dict)
    
    fun_lookup["DICT_STORE_SUBSCRIPT"] = process_dict_store_subscript

    def process_dict_delete_subscript(heap_id, key):
        a_dict = heap_id_to_object_dict[heap_id]
        new_dict = a_dict.copy()
        del new_dict[key]
        update_heap_object(heap_id, new_dict)
    
    fun_lookup["DICT_DELETE_SUBSCRIPT"] = process_dict_delete_subscript

    def process_dict_clear(heap_id):
        update_heap_object(heap_id, {})

    fun_lookup["DICT_CLEAR"] = process_dict_clear

    def process_dict_pop(heap_id, key):
        a_dict = heap_id_to_object_dict[heap_id]
        new_dict = a_dict.copy()
        new_dict.pop(key)
        update_heap_object(heap_id, new_dict)
    
    fun_lookup["DICT_POP"] = process_dict_pop
    fun_lookup["DICT_POP_ITEM"] = process_dict_pop

    def process_dict_setdefault(heap_id, key, value):
        a_dict = heap_id_to_object_dict[heap_id]
        new_dict = a_dict.copy()
        new_dict.setdefault(key, value)
        update_heap_object(heap_id, new_dict)
    
    fun_lookup["DICT_SET_DEFAULT"] = process_dict_setdefault

    def process_new_set(heap_id, *items):
        a_set = set(items)
        update_heap_object(heap_id, a_set)
    
    fun_lookup["NEW_SET"] = process_new_set
    fun_lookup["SET_UPDATE"] = process_new_set

    def process_set_add(heap_id, item):
        a_set = heap_id_to_object_dict[heap_id]
        new_set = a_set.copy()
        new_set.add(item)
        update_heap_object(heap_id, new_set)
    
    fun_lookup["SET_ADD"] = process_set_add

    def process_set_clear(heap_id):
        update_heap_object(heap_id, set())
    
    fun_lookup["SET_CLEAR"] = process_set_clear

    def process_set_discard(heap_id, item):
        a_set = heap_id_to_object_dict[heap_id]
        new_set = a_set.copy()
        new_set.discard(item)
        update_heap_object(heap_id, new_set)

    fun_lookup["SET_DISCARD"] = process_set_discard

    def process_new_object(heap_id, type_object):
        # represent an object simply with a dict
        update_heap_object(heap_id, {})
    
    fun_lookup["NEW_OBJECT"] = process_new_object

    def process_store_attr(heap_id, name, value):
        if heap_id not in heap_id_to_object_dict:
            return
        an_obj = heap_id_to_object_dict[heap_id]
        # objects represented by dicts
        new_obj = an_obj.copy()
        new_obj[name] = value
        update_heap_object(heap_id, new_obj)

    fun_lookup["STORE_ATTR"] = process_store_attr
            
    activate_snapshots = False
    cursor = conn.cursor()
    next_snapshot_id = 1
    next_fun_call_id = 1
    next_object_id = 1
    next_code_file_id = 1
    code_files = {}
    stack = None
    # memory address in original program (heap ID) => object in this program
    heap_id_to_object_dict = {}
    # memory address in this program => immutable object ID
    object_id_to_immutable_id_dict = {}
    # memory address in original program (heap ID) => immutable object ID
    heap_version = 1

    curr_line_no = None
    log_line_no = 0
    empty_list_oid = _save_object([], new_obj_id())
    empty_dict_oid = _save_object({}, new_obj_id())
    empty_set = set()
    empty_set_oid = _save_object(empty_set, new_obj_id())
    
    file = open(filename, "r")
    for line in file:
        log_line_no += 1
        if log_line_no % 100 == 0:
            print("\rLine " + str(log_line_no), end='')
        if line.startswith("--"):
            continue
        command = parse_line(line)
        if command[0] not in fun_lookup:
            print("Warning: no process function for command %s on line %d" % (command[0], log_line_no))
            continue
        fun = fun_lookup[command[0]]
        try:
            fun(*command[1])
            if log_line_no % 500 == 0:
                conn.commit()
        except Exception as e:
            print()
            print("Exception caught on line", log_line_no, line)
            raise e
    conn.commit()
    print()
    print("Complete")

def main():
    if len(sys.argv) < 2:
        print("Please provide a .rewind file.")
        return
    filename = sys.argv[1]
    
    sqlite_filename = re.sub("\.rewind$", ".sqlite", filename)
    print("Reading from " + filename)
    print("Recreating past states into " + sqlite_filename)
    if os.path.isfile(sqlite_filename):
        os.remove(sqlite_filename)
    conn = sqlite3.connect(sqlite_filename)
    define_schema(conn)
    recreate_past(conn, filename)

main()