# Todo

# lists
# tuples
# dicts
# sets
# objects

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
        return HeapRef(value[1:])
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
NEW_LIST_REGEX = re.compile(r'NEW_LIST\(((?:[*0-9]+, )+(?:[*0-9]+)?)\)')
STORE_FAST_REGEX = re.compile(r'STORE_FAST\(([0-9]+), (.*)\)')
STORE_NAME_REGEX = re.compile(r'STORE_NAME\((.*), (.*)\)')
LIST_APPEND_REGEX = re.compile(r'LIST_APPEND\((.*), (.*)\)')

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
        nonlocal next_object_id
        nonlocal cursor
        nonlocal conn
        nonlocal heap

        m = NEW_LIST_REGEX.match(line)
        if m:
            parts = m.group(1).split(', ')
            heap_id = int(parts[0])
            a_list = []
            heap_id_to_object_dict[heap_id] = a_list
            oid = save_object(a_list)
            heap = { **heap, heap_id: ObjectRef(oid) }
            save_object(heap)
            items = []
            return True
        return False
    
    def process_list_append(line):
        nonlocal stack
        nonlocal heap

        m = LIST_APPEND_REGEX.match(line)
        if m:
            heap_id = int(m.group(1))
            item = parse_value(m.group(2))
            a_list = heap_id_to_object_dict[heap_id]
            a_list = a_list + [item]
            heap_id_to_object_dict[heap_id] = a_list
            oid = save_object(a_list)
            heap = { **heap, heap_id: ObjectRef(oid) }
            save_object(heap)
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
    
    file = open("rewind.log", "r")
    for line in file:
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
        if process_list_append(line):
            continue

def main():
    if os.path.isfile("rewind.sqlite"):
        os.remove("rewind.sqlite")
    conn = sqlite3.connect('rewind.sqlite')
    define_schema(conn)
    recreate_past(conn)

main()