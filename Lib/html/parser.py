import re
import _markupbase
from html import unescape

class HTMLParser(_markupbase.ParserBase):
    def __init__(self, *, convert_charrefs=True):
        super().__init__()
        self.convert_charrefs = convert_charrefs
        self.reset()

    def reset(self):
        super().reset()
        self.rawdata = ''
        self.lasttag = '???'
        self.interesting = re.compile('[&<]')
        self.cdata_elem = None

    def handle_starttag(self, tag, attrs):
        """Handle start tag"""
        print(f"Start Tag: <{tag}> with attributes: {attrs}")

    def handle_endtag(self, tag):
        """Handle end tag"""
        print(f"End Tag: </{tag}>")

    def handle_charref(self, name):
        """Handle character reference"""
        char_num = int(name)
        char = chr(char_num)
        print(f"Character Reference: {char} (Numeric: {char_num})")

    def handle_entityref(self, name):
        """Handle entity reference"""
        print(f"Entity Reference: &{name};")

    def handle_data(self, data):
        """Handle data"""
        print(f"Data: {data}")

    def handle_comment(self, data):
        """Handle comment"""
        print(f"Comment: <!--{data}-->")

    def handle_decl(self, decl):
        """Handle declaration"""
        print(f"Declaration: <!{decl}>")

    def handle_pi(self, data):
        """Handle processing instruction"""
        print(f"Processing Instruction: <?{data}>")

    def unknown_decl(self, data):
        """Handle unknown declaration"""
        print(f"Unknown Declaration: {data}")

    def handle_startendtag(self, tag, attrs):
        """Handle start-end tag"""
        print(f"Start-End Tag: <{tag}/> with attributes: {attrs}")

    def set_cdata_mode(self, elem):
        """Set CDATA mode"""
        self.cdata_elem = elem.lower()
        self.interesting = re.compile(r'</\s*%s\s*>' % self.cdata_elem, re.I)

    def clear_cdata_mode(self):
        """Clear CDATA mode"""
        self.interesting = re.compile('[&<]')
        self.cdata_elem = None

    def parse_starttag(self, i):
        """Parse start tag"""
        rawdata = self.rawdata
        endpos = self.check_for_whole_start_tag(i)
        if endpos < 0:
            return endpos

        tag = self.lasttag
        attrs = []

        match = re.match(r'<([a-zA-Z][^\t\n\r\f />\x00]*)', rawdata[i:endpos])
        if match:
            tag = match.group(1).lower()

        attr_match = re.findall(r'([^\s/>][^\s/=>]*)(\s*=\s*("[^"]*"|\'[^\']*\'|[^\s/=>]+))?', rawdata[i:endpos])
        for attr_name, _, attr_value in attr_match:
            if attr_value is not None:
                attr_value = unescape(attr_value.strip('"\''))
            attrs.append((attr_name.lower(), attr_value))

        self.handle_starttag(tag, attrs)
        return endpos

    def parse_endtag(self, i):
        """Parse end tag"""
        rawdata = self.rawdata
        match = re.match(r'</\s*([a-zA-Z][-.a-zA-Z0-9:_]*)\s*>', rawdata[i:])
        if not match:
            return -1

        tag = match.group(1).lower()
        self.handle_endtag(tag)
        return i + match.end()

    def parse_pi(self, i):
        """Parse processing instruction"""
        rawdata = self.rawdata
        match = re.match(r'<\?(.*?)\?>', rawdata[i:], re.DOTALL)
        if not match:
            return -1

        pi_data = match.group(1).strip()
        self.handle_pi(pi_data)
        return i + match.end()

    def parse_bogus_comment(self, i, report=1):
        """Parse bogus comment"""
        rawdata = self.rawdata
        match = re.match(r'<!--(.*?)-->', rawdata[i:], re.DOTALL)
        if not match:
            return -1

        comment_data = match.group(1).strip()
        if report:
            self.handle_comment(comment_data)
        return i + match.end()

    def parse_html_declaration(self, i):
        """Parse HTML declaration"""
        rawdata = self.rawdata
        match = re.match(r'<!\s*([a-zA-Z][^\t\n\r\f />\x00]*)\s*(.*?)>', rawdata[i:], re.DOTALL)
        if not match:
            return -1

        declaration_type = match.group(1).lower()
        declaration_data = match.group(2).strip()
        self.handle_decl(f"{declaration_type} {declaration_data}")
        return i + match.end()

    def check_for_whole_start_tag(self, i):
        """Check for a complete start tag"""
        rawdata = self.rawdata
        match = re.match(r'<[a-zA-Z][^\t\n\r\f />\x00]*', rawdata[i:])
        if not match:
            return -1

        tag = match.group(0)
        endpos = i + len(tag)
        if rawdata[endpos:endpos + 1] == '/':
            return endpos + 1  # Self-closing tag
        elif rawdata[endpos:endpos + 1] == '>':
            return endpos + 1  # Complete tag
        else:
            return -1

    def goahead(self, end):
        """Handle data as far as reasonable"""
        rawdata = self.rawdata
        i = 0
        n = len(rawdata)
        while i < n:
            # ... Implement the logic for handling data, comments, etc. ...
            pass
            i += 1

# Usage example:
if __name__ == "__main__":
    parser = HTMLParser()
    with open("sample.html", "r") as html_file:
        html_data = html_file.read()
        parser.feed(html_data)
    parser.close()
