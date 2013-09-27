"""Bring time stamps of generated checked-in files into the right order

A versioned configuration file .hgtouch specifies generated files, in the
syntax of make rules.

  output:    input1 input2

In addition to the dependency syntax, #-comments are supported.
"""
from __future__ import with_statement
import errno
import os

def parse_config(repo):
    try:
        fp = repo.wfile(".hgtouch")
    except IOError, e:
        if e.errno != errno.ENOENT:
            raise
        return {}
    result = {}
    with fp:
        for line in fp:
            # strip comments
            line = line.split('#')[0].strip()
            if ':' not in line:
                continue
            outputs, inputs = line.split(':', 1)
            outputs = outputs.split()
            inputs = inputs.split()
            for o in outputs:
                try:
                    result[o].extend(inputs)
                except KeyError:
                    result[o] = inputs
    return result

def check_rule(ui, repo, modified, output, inputs):
    f_output = repo.wjoin(output)
    try:
        o_time = os.stat(f_output).st_mtime
    except OSError:
        ui.warn("Generated file %s does not exist\n" % output)
        return False
    need_touch = False
    backdate = None
    backdate_source = None
    for i in inputs:
        f_i = repo.wjoin(i)
        try:
            i_time = os.stat(f_i).st_mtime
        except OSError:
            ui.warn(".hgtouch input file %s does not exist\n" % i)
            return False
        if i in modified:
            # input is modified. Need to backdate at least to i_time
            if backdate is None or backdate > i_time:
                backdate = i_time
                backdate_source = i
            continue
        if o_time <= i_time:
            # generated file is older, touch
            need_touch = True
    if backdate is not None:
        ui.warn("Input %s for file %s locally modified\n" % (backdate_source, output))
        # set to 1s before oldest modified input
        backdate -= 1
        os.utime(f_output, (backdate, backdate))
        return False
    if need_touch:
        ui.note("Touching %s\n" % output)
        os.utime(f_output, None)
    return True

def do_touch(ui, repo):
    modified = repo.status()[0]
    dependencies = parse_config(repo)
    success = True
    # try processing all rules in topological order
    hold_back = {}
    while dependencies:
        output, inputs = dependencies.popitem()
        # check whether any of the inputs is generated
        for i in inputs:
            if i in dependencies:
                hold_back[output] = inputs
                continue
        success = check_rule(ui, repo, modified, output, inputs)
        # put back held back rules
        dependencies.update(hold_back)
        hold_back = {}
    if hold_back:
        ui.warn("Cyclic dependency involving %s\n" % (' '.join(hold_back.keys())))
        return False
    return success

def touch(ui, repo):
    "touch generated files that are older than their sources after an update."
    do_touch(ui, repo)

cmdtable = {
    "touch": (touch, [],
              "touch generated files according to the .hgtouch configuration")
}
