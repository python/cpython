"""macgen_info - Generate informational output"""

def generate(output, module_dict):
    for name in module_dict.keys():
        print 'Include %-20s\t'%name,
        module = module_dict[name]
        print module.gettype(), '\t', repr(module)
    return 0
