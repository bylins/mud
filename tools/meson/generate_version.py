#!/usr/bin/env python3
import sys

input_file, output_file, git_rev, buildtype, build_features, build_compiler = sys.argv[1:]

with open(input_file, encoding='koi8-r') as f:
    content = f.read()

content = content.replace('${REVISION}', f'{git_rev} ({buildtype})')
content = content.replace('${BUILD_COMPILER}', build_compiler)
content = content.replace('${BUILD_FEATURES}', build_features)

with open(output_file, 'w', encoding='koi8-r') as f:
    f.write(content)