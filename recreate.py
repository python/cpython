# Todo

# dicts
# sets
# objects
# make line parsing generic
# perf of recreate.py (probably transactions) https://docs.python.org/3/library/profile.html#module-cProfile

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
            
            constraint FunCall_fk_parent_id foreign key (parent_id)
                references FunCall(id)
        );
    """)

    c.execute(""" 
        create table SourceCode (
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

def parse_csv(value):
    parts = value.split(", ")
    if parts == [""]:
        return []
    return list(map(parse_value, parts))

def parse_value(value):
    if value[0] == "*":
        return HeapRef(int(value[1:]))
    else:
        return eval(value)

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

class FunCall(object):
    def __init__(self, id, filename, name, varnames, locals):
        self.id = id
        self.filename = filename
        self.name = name
        self.varnames = varnames

    def __repr__(self):
        return "<" + str(self.id) + ">" + self.name + "(" + str(self.varnames) + ")"

VISIT_REGEX = re.compile(r'VISIT #([0-9]+)')
PUSH_FRAME_REGEX = re.compile(r'PUSH_FRAME\(filename: (.*), name: (.*), varnames: \((.*)\), locals: \((.*)\)\)')
RETURN_VALUE_REGEX = re.compile(r'RETURN_VALUE\((.*)\)')
DEALLOC_OBJECT_REGEX = re.compile(r'DEALLOC_OBJECT\((.*)\)')
NEW_LIST_REGEX = re.compile(r'NEW_LIST\((.*)\)')
NEW_STRING_REGEX = re.compile(r'NEW_STRING\((.*), (.*)\)')
STORE_FAST_REGEX = re.compile(r'STORE_FAST\(([0-9]+), (.*)\)')
STORE_NAME_REGEX = re.compile(r'STORE_NAME\((.*), (.*)\)')
LIST_APPEND_REGEX = re.compile(r'LIST_APPEND\((.*), (.*)\)')
LIST_EXTEND_REGEX = re.compile(r'LIST_EXTEND\((.*), (.*)\)')
NEW_TUPLE_REGEX = re.compile(r'NEW_TUPLE\((.*?), (.*)\)')
LIST_DELETE_SUBSCRIPT_REGEX = re.compile(r'LIST_DELETE_SUBSCRIPT\((.*), (.*)\)')
LIST_STORE_SUBSCRIPT_REGEX = re.compile(r'LIST_STORE_SUBSCRIPT\((.*), (.*), (.*)\)')
LIST_INSERT_REGEX = re.compile(r'LIST_INSERT\((.*), (.*), (.*)\)')
LIST_REMOVE_REGEX = re.compile(r'LIST_REMOVE\((.*), (.*)\)')
LIST_POP_REGEX = re.compile(r'LIST_POP\((.*), (.*)\)')
LIST_CLEAR_REGEX = re.compile(r'LIST_CLEAR\((.*)\)')
LIST_REVERSE_REGEX = re.compile(r'LIST_REVERSE\((.*)\)')
LIST_SORT_REGEX = re.compile(r'LIST_SORT\((.*)\)')
STRING_INPLACE_ADD_RESULT_REGEX = re.compile(r'STRING_INPLACE_ADD_RESULT\((.*), (.*)\)')

def recreate_past(conn):
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
        return quote(str(item[0])) + ": " + serialize_member(item[1])

    def serialize(value):
        if hasattr(value, "serialize"):
            return value.serialize()
        if isinstance(value, list):
            return "[" + ", ".join(map(serialize_member, value)) + "]"
        elif isinstance(value, dict):
            return "{" + ", ".join(map(format_key_value_pair, value.items())) + "}"
        elif isinstance(value, tuple):
            return "[" + ", ".join(map(serialize_member, value)) + "]"
        else:
            return serialize_member(value)

    def save_object(obj):
        nonlocal object_id_to_immutable_id_dict

        oid = new_obj_id()
        object_id_to_immutable_id_dict[id(obj)] = oid
        cursor.execute("INSERT INTO Object VALUES (?, ?)", (
            oid,
            serialize(obj)
        ))
        conn.commit()
        return oid
    
    def update_heap_object(heap_id, new_obj):
        nonlocal heap
        nonlocal heap_id_to_object_dict

        if heap_id == 4481524480:
            print("found it", log_line_no)
        heap_id_to_object_dict[heap_id] = new_obj
        oid = save_object(new_obj)
        heap = { **heap, heap_id: ObjectRef(oid) }
        save_object(heap)

    def process_push_frame(line):
        nonlocal begin
        nonlocal conn
        nonlocal conn
        nonlocal source_code_files
        nonlocal stack
        nonlocal next_source_code_id
        
        m = PUSH_FRAME_REGEX.match(line)
        if m:
            filename = m.group(1)
            name = m.group(2)
            if name == "<module>":
                begin = True
            else:
                if not begin:
                    return True
            varnames = parse_csv(m.group(3))
            locals = parse_csv(m.group(4))
            
            parent_id = None
            if stack is not None:
                parent_id = stack[0].fun_call.id
            
            if filename not in source_code_files and os.path.isfile(filename):
                the_file = open(filename, "r")
                content = the_file.read()
                the_file.close()
                source_code_id = next_source_code_id
                next_source_code_id += 1
                cursor.execute("INSERT INTO SourceCode VALUES (?, ?, ?)", (
                    source_code_id,
                    filename,
                    content
                ))
                conn.commit()
                source_code_files.add(filename)
            
            fun_call = FunCall(new_fun_call_id(), filename, name, varnames, locals)
            frame_vars = StackFrameVars(locals, varnames)
            save_object(frame_vars)
            frame = StackFrame(fun_call, frame_vars)
            save_object(frame)
            stack = [frame, stack]
            save_object(stack)

            cursor.execute("INSERT INTO FunCall VALUES (?, ?, ?, ?)", (
                fun_call.id,
                name,
                immut_id(frame_vars),
                parent_id,
            ))

            conn.commit()
            return True
        return False

    def process_visit(line):
        nonlocal stack
        nonlocal cursor
        nonlocal conn
        nonlocal heap
        nonlocal line_no

        m = VISIT_REGEX.match(line)
        if m:
            line_no = int(m.group(1))
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
            return True
        return False

    def process_store_name(line):
        nonlocal stack

        m = STORE_NAME_REGEX.match(line)
        if m:
            name = eval(m.group(1))
            value = parse_value(m.group(2))
            new_frame_vars = stack[0].frame_vars.update_by_name(name, value)
            save_object(new_frame_vars)
            new_frame = StackFrame(stack[0].fun_call, new_frame_vars)
            save_object(new_frame)
            stack = [new_frame, stack[1]]
            save_object(stack)
            return True
        return False
    
    def process_store_fast(line):
        nonlocal stack

        m = STORE_FAST_REGEX.match(line)
        if m:
            index = int(m.group(1))
            value = parse_value(m.group(2))
            assert stack is not None
            new_frame_vars = stack[0].frame_vars.update_local(index, value)
            save_object(new_frame_vars)
            new_frame = StackFrame(stack[0].fun_call, new_frame_vars)
            save_object(new_frame)
            stack = [new_frame, stack[1]]
            save_object(stack)
            return True
        return False

    def process_dealloc_object(line):
        m = DEALLOC_OBJECT_REGEX.match(line)
        if m:
            object_ids = map(int, m.group(1).split(", "))
            for oid in object_ids:
                object_tracker.untrack(oid)
            return True
        return False
    
    def process_return_value(line):
        nonlocal stack
        nonlocal cursor
        nonlocal conn
        nonlocal heap

        m = RETURN_VALUE_REGEX.match(line)
        if m:
            ret_val = parse_value(m.group(1))
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
                line_no, 
                None
            ))
            conn.commit()

            stack = stack[1]
            ret_val = m.group(1)
            return True
        return False
    
    def process_new_list(line):
        m = NEW_LIST_REGEX.match(line)
        if m:
            parts = list(filter(lambda part: part != '', m.group(1).split(', ')))
            heap_id = int(parts[0])
            a_list = list(map(parse_value, parts[1:]))
            update_heap_object(heap_id, a_list)
            return True
        return False
    
    def process_new_string(line):
        m = NEW_STRING_REGEX.match(line)
        if m:
            heap_id = m.group(1)
            string = parse_value(m.group(2))
            update_heap_object(heap_id, string)
            return True
        return False
    
    def process_list_append(line):
        nonlocal stack
        nonlocal heap

        m = LIST_APPEND_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            item = parse_value(m.group(2))
            if heap_id in heap_id_to_object_dict:
                a_list = heap_id_to_object_dict[heap_id]
                a_list = a_list + [item]
                update_heap_object(heap_id, a_list)
            return True
        return False
    
    def process_list_extend(line):
        nonlocal stack
        nonlocal heap

        m = LIST_EXTEND_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            other_list_heap_ref = parse_value(m.group(2))
            a_list = heap_id_to_object_dict[heap_id]
            other_list = heap_id_to_object_dict[other_list_heap_ref.id]
            new_a_list = a_list + list(other_list)
            update_heap_object(heap_id, new_a_list)
            return True
        return False
    
    def process_new_tuple(line):
        nonlocal stack
        nonlocal heap

        m = NEW_TUPLE_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            items = parse_csv(m.group(2))
            a_tuple = tuple(items)
            update_heap_object(heap_id, a_tuple)
            return True
        return False

    def process_list_delete_subscript(line):
        nonlocal stack
        nonlocal heap
        m = LIST_DELETE_SUBSCRIPT_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            index = parse_value(m.group(2))
            obj = heap_id_to_object_dict[heap_id]
            new_obj = obj.copy()
            del new_obj[index]
            update_heap_object(heap_id, new_obj)
            return True
        return False

    def process_list_store_subscript(line):
        nonlocal stack
        nonlocal heap
        m = LIST_STORE_SUBSCRIPT_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            index = parse_value(m.group(2))
            value = parse_value(m.group(3))
            obj = heap_id_to_object_dict[heap_id]
            # lists only. TODO support dict and other structures
            new_list = obj.copy()
            new_list[index] = value
            update_heap_object(heap_id, new_list)
            return True
        return False

    def process_list_insert(line):
        m = LIST_INSERT_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            index = parse_value(m.group(2))
            value = parse_value(m.group(3))
            obj = heap_id_to_object_dict[heap_id]
            new_list = obj.copy()
            new_list.insert(index, value)
            update_heap_object(heap_id, new_list)
            return True
        return False

    def process_list_remove(line):
        m = LIST_REMOVE_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            value = parse_value(m.group(2))
            obj = heap_id_to_object_dict[heap_id]
            new_list = obj.copy()
            new_list.remove(value)
            update_heap_object(heap_id, new_list)
            return True
        return False

    def process_list_pop(line):
        m = LIST_POP_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            index = int(m.group(2))
            a_list = heap_id_to_object_dict[heap_id]
            new_list = a_list.copy()
            new_list.pop(index)
            update_heap_object(heap_id, new_list)
            return True
        return False

    def process_list_clear(line):
        m = LIST_CLEAR_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            update_heap_object(heap_id, [])
            return True
        return False

    def process_list_reverse(line):
        m = LIST_REVERSE_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            a_list = heap_id_to_object_dict[heap_id]
            new_list = a_list.copy()
            new_list.reverse()
            update_heap_object(heap_id, new_list)
            return True
        return False
    
    def process_list_sort(line):
        m = LIST_SORT_REGEX.match(line)
        if m:
            parts = parse_csv(m.group(1))
            heap_id = parts[0]
            new_list = parts[1:]
            update_heap_object(heap_id, new_list)
            return True
        return False

    def process_string_inplace_add_result(line):
        m = STRING_INPLACE_ADD_RESULT_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            string = parse_value(m.group(2))
            update_heap_object(heap_id, string)
            return True
        return False
    
    
            
    cursor = conn.cursor()
    begin = False
    next_snapshot_id = 1
    next_fun_call_id = 1
    next_object_id = 1
    next_source_code_id = 1
    source_code_files = set()
    stack = None
    # memory address in original program (heap ID) => object in this program
    heap_id_to_object_dict = {}
    # memory address in this program => immutable object ID
    object_id_to_immutable_id_dict = {}
    # memory address in original program (heap ID) => immutable object ID
    heap = {}

    save_object(heap)
    line_no = None
    log_line_no = 0
    
    file = open("rewind.log", "r")
    for line in file:
        log_line_no += 1
        if process_push_frame(line):
            continue
        if not begin:
            continue
        if process_visit(line):
            continue
        if process_store_name(line):
            continue
        if process_store_fast(line):
            continue
        if process_return_value(line):
            continue
        if process_new_list(line):
            continue
        if process_new_string(line):
            continue
        if process_list_append(line):
            continue
        if process_list_extend(line):
            continue
        if process_new_tuple(line):
            continue
        if process_list_delete_subscript(line):
            continue
        if process_list_store_subscript(line):
            continue
        if process_list_insert(line):
            continue
        if process_list_remove(line):
            continue
        if process_list_pop(line):
            continue
        if process_list_clear(line):
            continue
        if process_list_reverse(line):
            continue
        if process_string_inplace_add_result(line):
            continue
        if process_list_sort(line):
            continue
        

def main():
    if os.path.isfile("rewind.sqlite"):
        os.remove("rewind.sqlite")
    conn = sqlite3.connect('rewind.sqlite')
    define_schema(conn)
    recreate_past(conn)

main()