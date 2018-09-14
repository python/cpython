#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse

from .frozen_module import Serializer


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='Functions')

    parser_1 = subparsers.add_parser('freeze-py',
                                     help="Freeze a single python script")
    parser_1.add_argument('py_file', type=str,
                          help='The python file to freeze')
    parser_1.add_argument('-o', '--out-file', type=str,
                          default='out.c', nargs='?',
                          help="The output filename (defaults to 'out.c')")
    parser_1.set_defaults(freeze_py=True)

    parser_2 = subparsers.add_parser('freeze-startup-modules',
                                     help="Freeze startup modules "
                                          "from a python interpreter")
    parser_2.set_defaults(freeze_startup_modules=True)
    parser_2.add_argument('python_binary', type=str,
                          help="The python interpreter to get "
                               "the startup python modules from")
    parser_2.add_argument('-o', '--out-file', type=str,
                          default='out.c', nargs='?',
                          help="The output filename (defaults to 'out.c')")
    parser_2.add_argument('-x', '--extra-module', action='append',
                           help="Extra modules to include")

    args = parser.parse_args()
    serializer = Serializer()
    if getattr(args, 'freeze_py', False):
        serializer.freeze_py(args.py_file, output_filename=args.out_file)
    elif getattr(args, 'freeze_startup_modules', False):
        serializer.freeze_startup_modules(args.python_binary,
                                          args.out_file,
                                          extra_modules=args.extra_module)
