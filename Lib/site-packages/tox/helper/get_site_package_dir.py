from __future__ import unicode_literals

import distutils.sysconfig
import json
import sys

data = json.dumps({"dir": distutils.sysconfig.get_python_lib(prefix=sys.argv[1])})
print(data)
