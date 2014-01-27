"""Bring time stamps of generated checked-in files into the right order

A versioned configuration file .hgtouch specifies generated files, in the
syntax of make rules.

  output:    input1 input2

In addition to the dependency syntax, #-comments are supported.
"""
from __future__ import with_statement
import errno
import os
import time

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

def check_rule(ui, repo, modified, basedir, output, inputs):
    """Verify that the output is newer than any of the inputs.
    Return (status, stamp), where status is True if the update succeeded,
    and stamp is the newest time stamp assigned  to any file (might be in
    the future).

    If basedir is nonempty, it gives a directory in which the tree is to
    be checked.
    """
    f_output = repo.wjoin(os.path.join(basedir, output))
    try:
        o_time = os.stat(f_output).st_mtime
    except OSError:
        ui.warn("Generated file %s does not exist\n" % output)
        return False, 0
    youngest = 0   # youngest dependency
    backdate = None
    backdate_source = None
    for i in inputs:
        f_i = repo.wjoin(os.path.join(basedir, i))
        try:
            i_time = os.stat(f_i).st_mtime
        except OSError:
            ui.warn(".hgtouch input file %s does not exist\n" % i)
            return False, 0
        if i in modified:
            # input is modified. Need to backdate at least to i_time
            if backdate is None or backdate > i_time:
                backdate = i_time
                backdate_source = i
            continue
        youngest = max(i_time, youngest)
    if backdate is not None:
        ui.warn("Input %s for file %s locally modified\n" % (backdate_source, output))
        # set to 1s before oldest modified input
        backdate -= 1
        os.utime(f_output, (backdate, backdate))
        return False, 0
    if youngest >= o_time:
        ui.note("Touching %s\n" % output)
        youngest += 1
        os.utime(f_output, (youngest, youngest))
        return True, youngest
    else:
        # Nothing to update
        return True, 0

def do_touch(ui, repo, basedir):
    if basedir:
        if not os.path.isdir(repo.wjoin(basedir)):
            ui.warn("Abort: basedir %r does not exist\n" % basedir)
            return
        modified = []
    else:
        modified = repo.status()[0]
    dependencies = parse_config(repo)
    success = True
    tstamp = 0       # newest time stamp assigned
    # try processing all rules in topological order
    hold_back = {}
    while dependencies:
        output, inputs = dependencies.popitem()
        # check whether any of the inputs is generated
        for i in inputs:
            if i in dependencies:
                hold_back[output] = inputs
                continue
        _success, _tstamp = check_rule(ui, repo, modified, basedir, output, inputs)
        success = success and _success
        tstamp = max(tstamp, _tstamp)
        # put back held back rules
        dependencies.update(hold_back)
        hold_back = {}
    now = time.time()
    if tstamp > now:
        # wait until real time has passed the newest time stamp, to
        # avoid having files dated in the future
        time.sleep(tstamp-now)
    if hold_back:
        ui.warn("Cyclic dependency involving %s\n" % (' '.join(hold_back.keys())))
        return False
    return success

def touch(ui, repo, basedir):
    "touch generated files that are older than their sources after an update."
    do_touch(ui, repo, basedir)

cmdtable = {
    "touch": (touch,
              [('b', 'basedir', '', 'base dir of the tree to apply touching', 'BASEDIR')],
              "hg touch [-b BASEDIR]")
}
