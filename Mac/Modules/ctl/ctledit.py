as_resource_body = """
return ResObj_New((Handle)_self->ob_itself);
"""

f = ManualGenerator("as_Resource", as_resource_body)
f.docstring = lambda : "Return this Control as a Resource"

methods.append(f)
