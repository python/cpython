import functools, re

class AsciiBuffer(object):
    
    def __init__(self):
        self.data = []
        
    def set(self, x, y, value):
        if len(self.data) < y:
            for i in range(y - len(self.data)):
                self.data.append([])
        row = self.data[y - 1]
        if len(row) < x:
            row.extend([' '] * (x - len(row)))
        row[x - 1] = value
    
    def serialize(self):
        output = ""
        begin_lines = True
        lines = []
        for line in self.data:
            if begin_lines:
                if len(list(filter(lambda c: c != " ", line))) > 0:
                    begin_lines = False
                    lines.append(line)
            else:
                lines.append(line)
        
        def reducer(i, line):
            iter = (i for i, x in enumerate(line) if x != ' ')
            try:
                idx = next(iter)
            except StopIteration:
                return i
            if i is None and idx is None:
                return None
            elif idx is None:
                return i
            elif i is None:
                return idx
            else:
                return min(i, idx)
        
        first_non_space_idx = functools.reduce(reducer, lines, None)
        #print("lines", lines, "first_non_space_idx", first_non_space_idx)
        return "\n".join(
            map(lambda row: "".join(row[first_non_space_idx:]), lines)
        )
        
    def deserialize(self, input, tsize):
        raw_lines = input.split('\n')
        control_code_pattern = re.compile("\x1B\[[0-9]+m(.?)")
        lines = []
        max_line_length = 0
        for j, raw_line in enumerate(raw_lines):
            line = []
            i = 0
            while i < len(raw_line):
                chr = raw_line[i]
                m = control_code_pattern.match(raw_line, i)
                if m:
                    if m.groups()[0] != '': # ignore resets
                        line.append(m.group())
                    i = m.end()
                    continue
                else:
                    line.append(chr)
                i += 1
            if j < len(raw_lines) - 1:
                lines.append(line)
                max_line_length = max(max_line_length, len(line))
        
        
        # add back white space
        num_top_blank_lines = (tsize.lines - len(lines)) // 2
        num_left_spaces = (tsize.columns - max_line_length) // 2
        lines = list(map(lambda l: [" "] * num_left_spaces + l, lines))
        for i in range(num_top_blank_lines):
            lines.insert(0, [])
        
        self.data = lines
    
    def __repr__(self):
        return "ASCII Buffer:\n" + self.serialize()
            
if __name__ == '__main__':
    import os
    b = AsciiBuffer()
    file = open("alpha.txt")
    content = file.read()
    file.close()
    tsize = os.get_terminal_size()
    b.deserialize(content, tsize)
    #b.set(2, 4, "E")
    #b.set(2, 5, "D")
    #b.set(2, 6, "C")
    #b.set(3, 6, "B")
    #b.set(4, 6, "A")
    print(b.serialize())
            