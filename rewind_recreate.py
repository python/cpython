# Todo

# Trim initialization code execution
# lists
# dicts
# sets

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
            constraint Snapshot_fk_fun_call_id foreign key (fun_call_id)
                references FunCall(id)
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

    conn.commit()

def serialize_value(value):
    if isinstance(value, bool):
        if value:
            return "true"
        else:
            return "false"
    if isinstance(value, int):
        return str(value)
    elif value is None:
        return "null"
    else:
        if isinstance(value, HeapRef):
            # TODO: support heap refs
            return "null"
        elif hasattr(value, "id"):
            return "*" + str(value.id)
        else:
            raise Exception("Don't know how to serialize value " + value)

class HeapRef(object):
    def __init__(self, id):
        self.id = id
    
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

class StackFrame(object):
    def __init__(self, id, fun_call, frame_vars):
        self.id = id
        self.fun_call = fun_call
        self.frame_vars = frame_vars

    def serialize(self):
        return '{ "funCall": ' + str(self.fun_call.id) + ', "variables": *' + str(self.frame_vars.id) + '}'

class StackFrameVars(object):
    def __init__(self, id, stack_locals, varnames, extra_vars = None):
        self.id = id
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
    
    def update_local(self, id, idx, value):
        new_stack_locals = self.stack_locals.copy()
        new_stack_locals[idx] = value
        return StackFrameVars(id, new_stack_locals, self.varnames)
    
    def update_by_name(self, id, varname, value):
        return StackFrameVars(id, self.stack_locals, self.varnames, { **self.vars_dict, varname: value })

    def serialize(self):
        output = "{"
        items = list(self.vars_dict.items())
        for i in range(len(items)):
            item = items[i]
            if i != 0:
                output += ", "
            key = item[0]
            value = item[1]
            output += '"' + key + '": '
            output += serialize_value(value)
        output += "}"
        return output

class StackNode(object):
    def __init__(self, id, frame, parent):
        self.id = id
        self.frame = frame
        self.parent = parent

    def serialize(self):
        if self.parent:
            return "[ *" + str(self.frame.id) + ", *" + str(self.parent.id) + " ]"
        else:
            return "[ *" + str(self.frame.id) + ", null ]"

class Heap(object):
    def __init__(self, id):
        self.id = id
        self.dict = {}
    
    def serialize(self):
        return "{}"

def recreate_past(conn):
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

    def save_object(obj):
        cursor.execute("INSERT INTO Object VALUES (?, ?)", (
            obj.id,
            obj.serialize()
        ))
        conn.commit()

    cursor = conn.cursor()
    VISIT_REGEX = re.compile(r'VISIT #([0-9]+)')
    PUSH_FRAME_REGEX = re.compile(r'PUSH_FRAME\(filename: (.*), name: (.*), varnames: \((.*)\), locals: (.*)\)')
    RETURN_VALUE_REGEX = re.compile(r'RETURN_VALUE\((.*)\)')
    DEALLOC_OBJECT_REGEX = re.compile(r'DEALLOC_OBJECT\((.*)\)')
    NEW_LIST_REGEX = re.compile(r'NEW_LIST\(((?:[*0-9]+, )+(?:[*0-9]+)?)\)')
    STORE_FAST_REGEX = re.compile(r'STORE_FAST\(([0-9]+), (.*)\)')
    STORE_NAME_REGEX = re.compile(r'STORE_NAME\((.*), (.*)\)')
    
    next_snapshot_id = 1
    next_fun_call_id = 1
    next_object_id = 1
    next_source_code_id = 1
    source_code_files = set()
    stack = None
    heap_id_to_id_dict = {}
    object_dict = {}

    heap = Heap(new_obj_id())
    save_object(heap)
    line_no = None
    
    file = open("rewind.log", "r")
    for line in file:
        print(line)
        m = VISIT_REGEX.match(line)
        if m:
            line_no = int(m.group(1))
            cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?, ?)", (
                new_snapshot_id(), 
                stack and stack.frame.fun_call.id, 
                stack.id, 
                heap.id,
                None,
                line_no
            ))
            conn.commit()
            continue
        m = PUSH_FRAME_REGEX.match(line)
        if m:
            filename = m.group(1)
            name = m.group(2)
            varnames = parse_varnames(m.group(3))
            locals = parse_locals(m.group(4))
            parent_id = None
            if stack is not None:
                parent_id = stack.frame.fun_call.id
            
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
            # print(f"Push Frame: filename: {filename}, name: {name}, varnames: {varnames}, locals: {locals}")
            
            fun_call = FunCall(new_fun_call_id(), filename, name, varnames, locals)
            frame_vars = StackFrameVars(new_obj_id(), locals, varnames)
            save_object(frame_vars)
            frame = StackFrame(new_obj_id(), fun_call, frame_vars)
            save_object(frame)
            stack = StackNode(new_obj_id(), frame, stack)
            save_object(stack)

            cursor.execute("INSERT INTO FunCall VALUES (?, ?, ?, ?)", (
                fun_call.id,
                name,
                frame_vars.id,
                parent_id,
            ))

            conn.commit()
            continue
        m = STORE_NAME_REGEX.match(line)
        if m:
            name = eval(m.group(1))
            value = m.group(2)
            if value[0] == "*":
                value = HeapRef(value[1:])
            else:
                value = eval(value)
            new_frame_vars = stack.frame.frame_vars.update_by_name(new_obj_id(), name, value)
            save_object(new_frame_vars)
            new_frame = StackFrame(new_obj_id(), stack.frame.fun_call, new_frame_vars)
            save_object(new_frame)
            stack = StackNode(new_obj_id(), new_frame, stack.parent)
            save_object(stack)
            continue
        m = STORE_FAST_REGEX.match(line)
        if m:
            index = int(m.group(1))
            value = m.group(2)
            if value[0] == "*":
                value = HeapRef(value[1:])
            else:
                value = int(value)
            assert stack is not None
            new_frame_vars = stack.frame.frame_vars.update_local(new_obj_id(), index, value)
            save_object(new_frame_vars)
            new_frame = StackFrame(new_obj_id(), stack.frame.fun_call, new_frame_vars)
            save_object(new_frame)
            stack = StackNode(new_obj_id(), new_frame, stack.parent)
            save_object(stack)
            continue
        # m = DEALLOC_OBJECT_REGEX.match(line)
        # if m:
        #     print("Dealloc object:", m.group(1))
        #     object_ids = map(int, m.group(1).split(", "))
        #     for oid in object_ids:
        #         object_tracker.untrack(oid)
        #     continue
        m = RETURN_VALUE_REGEX.match(line)
        if m:
            ret_val = m.group(1)
            if ret_val[0] == "*":
                ret_val = None
            else:
                ret_val = eval(ret_val)
            new_frame_vars = stack.frame.frame_vars.update_by_name(new_obj_id(), "<ret val>", ret_val)
            save_object(new_frame_vars)
            new_frame = StackFrame(new_obj_id(), stack.frame.fun_call, new_frame_vars)
            save_object(new_frame)
            stack = StackNode(new_obj_id(), new_frame, stack.parent)
            save_object(stack)

            cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?, ?)", (
                new_snapshot_id(), 
                stack and stack.frame.fun_call.id, 
                stack.id, 
                heap.id,
                None,
                line_no
            ))
            conn.commit()

            stack = stack.parent
            ret_val = m.group(1)
            
            continue
        # m = NEW_LIST_REGEX.match(line)
        # if m:
        #     object_id = next_object_id
        #     next_object_id += 1
        #     parts = m.group(1).split(', ')
        #     if parts[-1] == '':
        #         # remove non-existent item at the end
        #         del parts[-1]
        #     heap_id = parts[0]
        #     heap_id_to_id_dict[heap_id] = object_id
        #     items = []
        #     for item in parts[1:]:
        #         if item[0] == '*':
        #             item_id = heap_id_to_id_dict[item[1:]]
        #             items.append('*' + str(item_id))
        #         else:
        #             items.append(item)
            
        #     jsonr = "[" + ", ".join(items) + "]"
        #     cursor.execute("INSERT INTO Object VALUES (?, ?)", (
        #         object_id,
        #         jsonr
        #     ))
        #     conn.commit()
        #     continue

def parse_varnames(varnames):
    if varnames == "":
        return []
    retval = []
    for part in varnames.split(", "):
        if part != "":
            retval.append(part[1:-1])
    return retval

def parse_locals(locals):
    parts = locals[1:-1].split(", ")
    locals = []
    for part in parts:
        if part == '':
            locals.append(None)
        elif part[0] == '*':
            locals.append(HeapRef(part[1:]))
        else:
            locals.append(eval(part))
    return locals

def main():
    os.remove("rewind.sqlite")
    conn = sqlite3.connect('rewind.sqlite')
    define_schema(conn)
    recreate_past(conn)

main()