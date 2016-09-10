import gc

class old_style_class():
    pass
class new_style_class(object):
    pass

a = old_style_class()
del a
gc.collect()
b = new_style_class()
del b
gc.collect()

a = old_style_class()
del old_style_class
gc.collect()
b = new_style_class()
del new_style_class
gc.collect()
del a
gc.collect()
del b
gc.collect()
