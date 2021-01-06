# Todo

# Solve the object ID reused problem
# Track return value of functions
# Trim initialization code execution
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
    def __init__(self, fun_call, locals):
        self.fun_call = fun_call
        self.locals = locals
        self.init_vars_dict()
    
    def init_vars_dict(self):
        self.vars_dict = {}
        for i in range(len(self.fun_call.varnames)):
            varname = self.fun_call.varnames[i]
            value = self.locals[i]
            self.vars_dict[varname] = value

    def update_local(self, idx, value):
        new_locals = self.locals.copy()
        new_locals[idx] = value
        return StackFrame(self.fun_call, new_locals)

class ObjectTracker(object):
    def __init__(self, db_conn, db_cursor):
        self.next_object_id = 1
        self.dict = {}
        self.db_conn = db_conn
        self.db_cursor = db_cursor
        self.objects = []
    
    def get_id(self, obj):
        return self.dict.get(id(obj))

    def assign_id(self):
        object_id = self.next_object_id
        self.next_object_id += 1
        return object_id

    def track(self, obj):
        self.objects.append(obj)
        if obj is None:
            return
        if id(obj) in self.dict:
            return
        object_id = self.assign_id()
        self.dict[id(obj)] = object_id
        # print("inserting object", object_id, self.serialize(obj))
        self.db_cursor.execute("INSERT INTO Object VALUES (?, ?)", (
            object_id,
            self.serialize(obj)
        ))
        self.db_conn.commit()

    def serialize_item(self, item):
        if isinstance(item, int):
            return str(item)
        elif isinstance(item, bool):
            if item:
                return "true"
            else:
                return "false"
        elif item is None:
            return "null"
        else:
            if id(item) not in self.dict:
                print("Item is not found in tracker: ", item, type(item))
                return "null"
            else:
                item_id = self.dict[id(item)]
                return "*" + str(item_id)

    def serialize(self, obj):
        if isinstance(obj, list):
            output = "["
            for i in range(len(obj)):
                item = obj[i]
                if i != 0:
                    output += ", "
                output += self.serialize_item(item)
            output += "]"
            return output
        elif isinstance(obj, dict):
            output = "{"
            items = list(obj.items())
            for i in range(len(items)):
                item = items[i]
                if i != 0:
                    output += ", "
                key = item[0]
                value = item[1]
                output += '"' + key + '": '
                output += self.serialize_item(value)
            output += "}"
            return output
        elif isinstance(obj, StackFrame):
            fun_call = obj.fun_call.id
            vars_dict_id = self.get_id(obj.vars_dict)
            return '{ "funCall": ' + str(fun_call) + ', "variables": *' + str(vars_dict_id) + '}'
        else:
            raise Exception("Unsupported type")

def recreate_past(conn):
    cursor = conn.cursor()
    VISIT_REGEX = re.compile(r'VISIT #([0-9]+)')
    PUSH_FRAME_REGEX = re.compile(r'PUSH_FRAME\(filename: (.*), name: (.*), varnames: \((.*)\), locals: (.*)\)')
    RETURN_VALUE_REGEX = re.compile(r'RETURN_VALUE\((.*)\)')
    NEW_LIST_REGEX = re.compile(r'NEW_LIST\(((?:[*0-9]+, )+(?:[*0-9]+)?)\)')
    STORE_FAST_REGEX = re.compile(r'STORE_FAST\(([0-9]+), (.*)\)')
    object_tracker = ObjectTracker(conn, cursor)
    heap = {}

    next_snapshot_id = 1
    next_fun_call_id = 1
    next_object_id = 1
    next_source_code_id = 1
    source_code_files = set()
    stack = None
    heap_id_to_id_dict = {}
    object_dict = {}

    object_tracker.track(heap)
    object_tracker.track(stack)
    
    file = open("rewind.log", "r")
    for line in file:
        print(line)
        m = VISIT_REGEX.match(line)
        if m:
            line = m.group(1)
            snapshot_id = next_snapshot_id
            next_snapshot_id += 1
            stack_id = object_tracker.get_id(stack)
            heap_id = object_tracker.get_id(heap)
            cursor.execute("INSERT INTO Snapshot VALUES (?, ?, ?, ?, ?, ?)", (
                snapshot_id, 
                stack and stack[0].fun_call.id, 
                stack_id, 
                heap_id,
                None,
                int(line)
            ))
            conn.commit()
            continue
        m = PUSH_FRAME_REGEX.match(line)
        if m:
            filename = m.group(1)
            name = m.group(2)
            varnames = parse_varnames(m.group(3))
            locals = parse_locals(m.group(4))
            fun_call_id = next_fun_call_id
            next_fun_call_id += 1
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
            # print(f"Push Frame: filename: {filename}, name: {name}, varnames: {varnames}, locals: {locals}")
            
            
            fun_call = FunCall(fun_call_id, filename, name, varnames, locals)
            print("push frame found:", fun_call)
            stack_frame_id = next_object_id
            next_object_id += 1
            frame = StackFrame(fun_call, locals)
            object_tracker.track(frame.vars_dict)
            object_tracker.track(frame)
            stack = [frame, stack]
            object_tracker.track(stack)

            parameters_id = object_tracker.get_id(frame.vars_dict)
            cursor.execute("INSERT INTO FunCall VALUES (?, ?, ?, ?)", (
                fun_call_id,
                name,
                parameters_id,
                parent_id,
            ))

            conn.commit()
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
            frame = stack[0].update_local(index, value)
            stack = [
                frame,
                stack[1]
            ]
            object_tracker.track(frame.vars_dict)
            object_tracker.track(frame)
            object_tracker.track(stack)
            continue
        m = RETURN_VALUE_REGEX.match(line)
        if m:
            stack = stack[1]
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