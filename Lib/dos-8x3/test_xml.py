'''Test module to thest the xmllib module.
   Sjoerd Mullender
'''

from test_support import verbose

testdoc = """\
<?xml version="1.0" encoding="UTF-8" standalone='yes' ?>
<!-- comments aren't allowed before the <?xml?> tag,
     but they are allowed before the <!DOCTYPE> tag -->
<!DOCTYPE greeting [
  <!ELEMENT greeting (#PCDATA)>
]>
<greeting>Hello, world!</greeting>
"""

import xmllib
if verbose:
	parser = xmllib.TestXMLParser()
else:
	parser = xmllib.XMLParser()

for c in testdoc:
	parser.feed(c)
parser.close()
