#!/usr/bin/env python3
import sys

input_file, output_file, git_rev, build_dir, buildtype = sys.argv[1:]

with open(input_file, encoding='koi8-r') as f:
    content = f.read()

content = content.replace('${REVISION}', f'{git_rev} ({buildtype})')
content = content.replace('${CMAKE_BINARY_DIR}', build_dir)

with open(output_file, 'w', encoding='koi8-r') as f:
    f.write(content)