#!/usr/bin/env python3
#
# Argument Clinic
# Copyright 2012-2013 by Larry Hastings.
# Licensed to the PSF under a contributor agreement.
#
from .utils import fail, ClinicError
from .function import Function, FunctionKind, Module, Class
from .block_printer import BlockPrinter, Destination
from .block_parser import Block, BlockParser
from .parameter import Parameter
from .clanguage import (
    linear_format, permute_left_option_groups,
    quoted_for_c_string, rstrip_lines, text_accumulator, unspecified,
    suffix_all_lines, strip_leading_and_trailing_blank_lines,
    normalize_snippet, indent_all_lines, format_escape,
    permute_right_option_groups, permute_optional_groups,
)
from .return_converter import (
    init_return_converter,
    int_return_converter,
)
from .converters import (
    NULL,
    int_converter,
    str_converter,
    self_converter,
)
from .clanguage import CLanguage
from .parser import DSLParser, version
from .clinic import Clinic, parse_file
from .cli import main
