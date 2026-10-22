#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path

JS_TEMPLATE = """
#include "emscripten.h"

EM_JS(void, {function_name}, (void), {{
    return new WebAssembly.Module(hexStringToUTF8Array("{hex_string}"));
}}
function hexStringToUTF8Array(hex) {{
  const bytes = [];
  for (let i = 0; i < hex.length; i += 2) {{
    bytes.push(parseInt(hex.substr(i, 2), 16));
  }}
  return new Uint8Array(bytes);
}});
"""


def prepare_wasm(input_file, output_file, function_name):
    # Read the compiled WASM as binary and convert to hex
    wasm_bytes = Path(input_file).read_bytes()

    hex_string = "".join(f"{byte:02x}" for byte in wasm_bytes)

    # Generate JavaScript module
    js_content = JS_TEMPLATE.format(
        function_name=function_name, hex_string=hex_string
    )
    Path(output_file).write_text(js_content)

    print(f"Successfully compiled {input_file} and generated {output_file}")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Compile WebAssembly text files using wasm-as"
    )
    parser.add_argument("input_file", help="Input .wat file to compile")
    parser.add_argument("output_file", help="Output file name")
    parser.add_argument("function_name", help="Name of the export function")

    args = parser.parse_args()

    return prepare_wasm(args.input_file, args.output_file, args.function_name)


if __name__ == "__main__":
    sys.exit(main())
