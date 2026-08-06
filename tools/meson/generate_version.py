#!/usr/bin/env python3
import sys
import subprocess
from datetime import datetime

input_file, output_file, git_rev, buildtype, build_features, build_compiler, source_root = sys.argv[1:]

# Ревизию берём на МОМЕНТ СБОРКИ, а не конфигурации. Этот скрипт запускается
# каждым ninja (build_always_stale), поэтому после `git pull` + `ninja` версия
# обновится сама -- без `meson setup --reconfigure` (issue #3472).
# Значение из meson (git_rev) остаётся запасным: если git недоступен (например,
# сборка из распакованного архива), используем его.
try:
    rev = subprocess.run(
        ['git', '-C', source_root, 'rev-parse', '--short', 'HEAD'],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
    if rev:
        git_rev = rev
except Exception:
    pass

# issue.versioning: версия движка = число коммитов в репозитории. Монотонна, +1 на коммит,
# считается на каждой сборке, как и хеш ревизии. Отдельного файла с номером в дереве нет:
# он только создавал конфликты в ветках и требовал ручных бампов. Если git недоступен
# (сборка из распакованного архива) -- 0, как и git_rev выше.
commit_count = "0"
try:
    cc = subprocess.run(
        ['git', '-C', source_root, 'rev-list', '--count', 'HEAD'],
        capture_output=True, text=True, check=True,
    ).stdout.strip()
    if cc:
        commit_count = cc
except Exception:
    pass

engine_name = "BRusMUD"
engine_version = commit_count

with open(input_file, encoding='koi8-r') as f:
    content = f.read()

content = content.replace('${REVISION}', f'{git_rev} ({buildtype})')
content = content.replace('${BUILD_COMPILER}', build_compiler)
content = content.replace('${BUILD_FEATURES}', build_features)
content = content.replace('${BUILD_DATETIME}', datetime.now().strftime('%b %d %Y %H:%M:%S'))
content = content.replace('${ENGINE_NAME}', engine_name)
content = content.replace('${ENGINE_VERSION}', engine_version)

with open(output_file, 'w', encoding='koi8-r') as f:
    f.write(content)
