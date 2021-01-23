# Todo

# clean up server.js code
# get classes to work as objects
# log exceptions
# learn how frozen works
# make log file same prefix name as program name
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
# optimization: perf of recreate.py (probably transactions) https://docs.python.org/3/library/profile.html#module-cProfile

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

def define_schema(conn):
    c = conn.cursor()

    c.execute("""
        create table Snapshot (
            id integer primary key,
            fun_call_id integer,
            stack integer,
            heap integer,
            interop integer,
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
            parameters integer,
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
        create table Error (
            id integer primary key,
            message text
        );
    """)

    conn.commit()

NUMBER_REGEX = r"-?(?:(?:[1-9][0-9]*)|[0-9])(?:\.[0-9]+)?"
STRING_REGEX = r"(?:'(?:[^'\\]|(?:\\['\\nt]))*')|(?:\"(?:[^\"\\]|(?:\\[\"\\nt]))*\")"
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
        return '{ "id": ' + str(self.id) + ' }'

    def __repr__(self):
        return f"HeapRef({self.id})"
    
    def __eq__(self, other):
        return self.id == other.id

    def __hash__(self):
        return self.id

class FunCall(object):
    def __init__(self, id, filename, code_file_id, name, varnames, locals):
        self.id = id
        self.filename = filename
        self.code_file_id = code_file_id
        self.name = name
        self.varnames = varnames

    def __repr__(self):
        return "<" + str(self.id) + ">" + self.name + "(" + str(self.varnames) + ")"

def recreate_past(conn):

    fun_lookup = {}

    class StackFrameVars(object):
        def __init__(self, stack_locals, varnames, extra_vars = None):
            self.varnames = varnames
            self.stack_locals = stack_locals
            self.init_vars_dict(stack_locals)
            if extra_vars:
                self.vars_dict.update(extra_vars)

        def init_vars_dict(self, stack_locals):
            self.vars_dict = {}
            for i in range(len(self.varnames)):
                varname = self.varnames[i]
                value = stack_locals[i]
                self.vars_dict[varname] = value
        
        def update_local(self, idx, value):
            new_stack_locals = self.stack_locals.copy()
            new_stack_locals[idx] = value
            return StackFrameVars(new_stack_locals, self.varnames)
        
        def update_by_name(self, varname, value):
            return StackFrameVars(self.stack_locals, self.varnames, { **self.vars_dict, varname: value })

        def serialize(self):
            return serialize(self.vars_dict)

    class StackFrame(object):
        def __init__(self, fun_call, frame_vars):
            self.fun_call = fun_call
            self.frame_vars = frame_vars

        def serialize(self):
            return '{ "funCall": ' + str(self.fun_call.id) + ', "variables": *' + str(immut_id(self.frame_vars)) + '}'
        
        def __repr__(self):
            return "Frame<" + repr(self.fun_call) + ">"

    def quote(value):
        return '"' + value.replace('"', '\\"') + '"'

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

        oid = new_obj_id()
        
        object_id_to_immutable_id_dict[id(obj)] = oid
        # print("saving object", oid, serialize(obj))
        cursor.execute("INSERT INTO Object VALUES (?, ?)", (
            oid,
            serialize(obj)
        ))
        conn.commit()
        return oid
    
    def update_heap_object(heap_id, new_obj):
        nonlocal heap
        nonlocal heap_id_to_object_dict

        heap_id_to_object_dict[heap_id] = new_obj
        oid = save_object(new_obj)
        
        heap = { **heap, str(heap_id): ObjectRef(oid) }
        save_object(heap)

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
            conn.commit()
            code_files[filename] = code_file_id

    def process_push_frame(filename, name, num_varnames, *rest):
        nonlocal stack
        nonlocal next_code_file_id
        nonlocal activate_snapshots
        
        varnames = []
        locals = []
        for i in range(num_varnames):
            varnames.append(rest[i])
        for i in range(num_varnames, len(rest)):
            locals.append(rest[i])

        parent_id = None
        if stack is not None:
            parent_id = stack[0].fun_call.id
        
        ensure_code_file_saved(filename, name)

        code_file_id = code_files.get(filename)
        fun_call = FunCall(new_fun_call_id(), filename, code_file_id, name, varnames, locals)
        frame_vars = StackFrameVars(locals, varnames)
        save_object(frame_vars)
        frame = StackFrame(fun_call, frame_vars)
        save_object(frame)
        stack = [frame, stack]
        save_object(stack)

        cursor.execute("INSERT INTO FunCall VALUES (?, ?, ?, ?, ?)", (
            fun_call.id,
            name,
            immut_id(frame_vars),
            parent_id,
            code_file_id
        ))

        conn.commit()
    
    fun_lookup["PUSH_FRAME"] = process_push_frame

    def process_visit(line_no):
        nonlocal curr_line_no

        curr_line_no = line_no
        if not activate_snapshots:
            return
        cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?, ?, ?)", (
            new_snapshot_id(), 
            stack and stack[0].fun_call.id, 
            immut_id(stack), 
            immut_id(heap),
            None,
            line_no,
            None
        ))
        conn.commit()
    
    fun_lookup["VISIT"] = process_visit

    def process_store_name(name, value):
        nonlocal stack
        new_frame_vars = stack[0].frame_vars.update_by_name(name, value)
        save_object(new_frame_vars)
        new_frame = StackFrame(stack[0].fun_call, new_frame_vars)
        save_object(new_frame)
        stack = [new_frame, stack[1]]
        save_object(stack)

    fun_lookup["STORE_NAME"] = process_store_name
    
    def process_store_fast(index, value):
        nonlocal stack
        assert stack is not None
        print("store fast into", stack)
        new_frame_vars = stack[0].frame_vars.update_local(index, value)
        save_object(new_frame_vars)
        new_frame = StackFrame(stack[0].fun_call, new_frame_vars)
        save_object(new_frame)
        stack = [new_frame, stack[1]]
        save_object(stack)
    
    fun_lookup["STORE_FAST"] = process_store_fast
    
    def process_return_value(ret_val, *extra_params):
        nonlocal stack
        if not activate_snapshots:
            return
        new_frame_vars = stack[0].frame_vars.update_by_name("<ret val>", ret_val)
        save_object(new_frame_vars)
        new_frame = StackFrame(stack[0].fun_call, new_frame_vars)
        save_object(new_frame)
        stack = [new_frame, stack[1]]
        save_object(stack)

        cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?, ?, ?)", (
            new_snapshot_id(), 
            stack and stack[0].fun_call.id, 
            immut_id(stack), 
            immut_id(heap),
            None,
            curr_line_no, 
            None
        ))
        conn.commit()

    fun_lookup["RETURN_VALUE"] = process_return_value
    fun_lookup["YIELD_VALUE"] = process_return_value

    def process_pop_frame(filename, name):
        nonlocal stack
        fun_call = stack[0].fun_call
        assert stack[0].fun_call.filename == filename
        assert stack[0].fun_call.name == name
        stack = stack[1]
    
    fun_lookup["POP_FRAME"] = process_pop_frame
    
    def process_new_list(heap_id, *a_list):
        update_heap_object(heap_id, list(a_list))

    fun_lookup["NEW_LIST"] = process_new_list
    
    def process_new_string(heap_id, string):
        print("process_new_string", heap_id, string)
        update_heap_object(heap_id, string)
    
    fun_lookup["NEW_STRING"] = process_new_string
    
    def process_list_append(heap_id, item):
        if heap_id in heap_id_to_object_dict:
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

    def process_list_delete_subscript(heap_id, index):
        obj = heap_id_to_object_dict[heap_id]
        new_obj = obj.copy()
        del new_obj[index]
        update_heap_object(heap_id, new_obj)
    
    fun_lookup["LIST_DELETE_SUBSCRIPT"] = process_list_delete_subscript

    def process_list_store_subscript(heap_id, index, value):
        obj = heap_id_to_object_dict[heap_id]
        new_list = obj.copy()
        new_list[index] = value
        update_heap_object(heap_id, new_list)
    
    fun_lookup["LIST_STORE_SUBSCRIPT"] = process_list_store_subscript

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
        if key in new_dict:
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

    def process_dealloc(heap_id):
        nonlocal heap
        if heap_id in heap_id_to_object_dict:
            obj = heap_id_to_object_dict[heap_id]
            del heap_id_to_object_dict[heap_id]
            del object_id_to_immutable_id_dict[id(obj)]
            new_heap = heap.copy()
            del new_heap[str(heap_id)]
            heap = new_heap
            save_object(heap)
    
    fun_lookup["DEALLOC"] = process_dealloc

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

    def process_new_object(heap_id, type_name, type_object):
        if type_name == 'type':
            # this is the definition of a custom class
            # do don't track anything here
            return
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
    heap = {}

    save_object(heap)
    curr_line_no = None
    log_line_no = 0
    
    file = open("rewind.log", "r")
    for line in file:
        log_line_no += 1
        print(log_line_no, line)
        if line.startswith("--"):
            continue
        command = parse_line(line)
        if command[0] not in fun_lookup:
            print("Warning: no process function for command %s on line %d" % (command[0], log_line_no))
            continue
        fun = fun_lookup[command[0]]
        fun(*command[1])

def main():
    if os.path.isfile("rewind.sqlite"):
        os.remove("rewind.sqlite")
    conn = sqlite3.connect('rewind.sqlite')
    define_schema(conn)
    recreate_past(conn)

main()