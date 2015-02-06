'''
Processes a CSV file containing a list of files into a WXS file with
components for each listed file.

The CSV columns are:
    source of file, target for file, group name

Usage::
    py txt_to_wxs.py [path to file list .csv] [path to destination .wxs]

This is necessary to handle structures where some directories only
contain other directories. MSBuild is not able to generate the
Directory entries in the WXS file correctly, as it operates on files.
Python, however, can easily fill in the gap.
'''

__author__ = "Steve Dower <steve.dower@microsoft.com>"

import csv
import re
import sys

from collections import defaultdict
from itertools import chain, zip_longest
from pathlib import PureWindowsPath
from uuid import uuid1

ID_CHAR_SUBS = {
    '-': '_',
    '+': '_P',
}

def make_id(path):
    return re.sub(
        r'[^A-Za-z0-9_.]',
        lambda m: ID_CHAR_SUBS.get(m.group(0), '_'),
        str(path).rstrip('/\\'),
        flags=re.I
    )

DIRECTORIES = set()

def main(file_source, install_target):
    with open(file_source, 'r', newline='') as f:
        files = list(csv.reader(f))

    assert len(files) == len(set(make_id(f[1]) for f in files)), "Duplicate file IDs exist"

    directories = defaultdict(set)
    cache_directories = defaultdict(set)
    groups = defaultdict(list)
    for source, target, group, disk_id, condition in files:
        target = PureWindowsPath(target)
        groups[group].append((source, target, disk_id, condition))

        if target.suffix.lower() in {".py", ".pyw"}:
            cache_directories[group].add(target.parent)

        for dirname in target.parents:
            parent = make_id(dirname.parent)
            if parent and parent != '.':
                directories[parent].add(dirname.name)

    lines = [
        '<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">',
        '    <Fragment>',
    ]
    for dir_parent in sorted(directories):
        lines.append('        <DirectoryRef Id="{}">'.format(dir_parent))
        for dir_name in sorted(directories[dir_parent]):
            lines.append('            <Directory Id="{}_{}" Name="{}" />'.format(dir_parent, make_id(dir_name), dir_name))
        lines.append('        </DirectoryRef>')
    for dir_parent in (make_id(d) for group in cache_directories.values() for d in group):
        lines.append('        <DirectoryRef Id="{}">'.format(dir_parent))
        lines.append('            <Directory Id="{}___pycache__" Name="__pycache__" />'.format(dir_parent))
        lines.append('        </DirectoryRef>')
    lines.append('    </Fragment>')

    for group in sorted(groups):
        lines.extend([
            '    <Fragment>',
            '        <ComponentGroup Id="{}">'.format(group),
        ])
        for source, target, disk_id, condition in groups[group]:
            lines.append('            <Component Id="{}" Directory="{}" Guid="*">'.format(make_id(target), make_id(target.parent)))
            if condition:
                lines.append('                <Condition>{}</Condition>'.format(condition))

            if disk_id:
                lines.append('                <File Id="{}" Name="{}" Source="{}" DiskId="{}" />'.format(make_id(target), target.name, source, disk_id))
            else:
                lines.append('                <File Id="{}" Name="{}" Source="{}" />'.format(make_id(target), target.name, source))
            lines.append('            </Component>')

        create_folders = {make_id(p) + "___pycache__" for p in cache_directories[group]}
        remove_folders = {make_id(p2) for p1 in cache_directories[group] for p2 in chain((p1,), p1.parents)}
        create_folders.discard(".")
        remove_folders.discard(".")
        if create_folders or remove_folders:
            lines.append('            <Component Id="{}__pycache__folders" Directory="TARGETDIR" Guid="{}">'.format(group, uuid1()))
            lines.extend('                <CreateFolder Directory="{}" />'.format(p) for p in create_folders)
            lines.extend('                <RemoveFile Id="Remove_{0}_files" Name="*" On="uninstall" Directory="{0}" />'.format(p) for p in create_folders)
            lines.extend('                <RemoveFolder Id="Remove_{0}_folder" On="uninstall" Directory="{0}" />'.format(p) for p in create_folders | remove_folders)
            lines.append('            </Component>')

        lines.extend([
            '        </ComponentGroup>',
            '    </Fragment>',
        ])
    lines.append('</Wix>')

    # Check if the file matches. If so, we don't want to touch it so
    # that we can skip rebuilding.
    try:
        with open(install_target, 'r') as f:
            if all(x.rstrip('\r\n') == y for x, y in zip_longest(f, lines)):
                print('File is up to date')
                return
    except IOError:
        pass

    with open(install_target, 'w') as f:
        f.writelines(line + '\n' for line in lines)
    print('Wrote {} lines to {}'.format(len(lines), install_target))

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
