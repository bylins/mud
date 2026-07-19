import sys
import os
import shutil

# Аргументы:
# 1: build_dir (где создавать папку small)
# 2: source_root (корень проекта, где лежат lib и lib.template)
# 3: full_world_path (опционально, путь для симлинка)

build_dir = sys.argv[1]
source_root = sys.argv[2]
full_world_path = sys.argv[3] if len(sys.argv) > 3 else ""

small_world_dir = os.path.join(build_dir, "small")

# Skip VCS metadata when copying world data: lib/world can be a git repo/submodule whose
# read-only packed objects both break the copy (EACCES on .git/objects) and don't belong in
# the build's runtime world. Matches any ".git" entry at any depth.
_IGNORE = shutil.ignore_patterns(".git")

print(f"Setting up world data in {build_dir}...")

lib_src = os.path.join(source_root, "lib")
template_src = os.path.join(source_root, "lib.template")

# Конфиги/тексты берём из lib (это актуальная боевая конфигурация), мир — из lib.template
# (свежий сконвертированный YAML). Всё остальное (plrs, plrobjs, etc, stat...) сервер
# создаёт сам при старте, так что small world получается абсолютно чистым.
LIB_DIRS = ("cfg", "text", "misc")
TEMPLATE_DIRS = ("world",)


def copy_dirs(src_root, names, what):
    for name in names:
        s = os.path.join(src_root, name)
        if not os.path.isdir(s):
            print(f"ERROR: '{name}' not found in {what} at {s}")
            sys.exit(1)
        d = os.path.join(small_world_dir, name)
        # Сносим прошлую копию, чтобы не оставалось мусора от старых конвертаций.
        shutil.rmtree(d, ignore_errors=True)
        shutil.copytree(s, d, ignore=_IGNORE)


if not os.path.exists(lib_src):
    print(f"ERROR: Source 'lib' not found at {lib_src}")
    sys.exit(1)

if not os.path.exists(template_src):
    print(f"ERROR: Source 'lib.template' not found at {template_src}")
    sys.exit(1)

os.makedirs(small_world_dir, exist_ok=True)

copy_dirs(lib_src, LIB_DIRS, "lib")
copy_dirs(template_src, TEMPLATE_DIRS, "lib.template")

print(f"Small world configured at: {small_world_dir}")

if full_world_path:
    full_world_dir = os.path.join(build_dir, "full")
    if os.path.exists(full_world_path):
        if not os.path.exists(full_world_dir):
            try:
                os.symlink(full_world_path, full_world_dir)
                print(f"Full world configured at: {full_world_dir} -> {full_world_path}")
            except OSError as e:
                print(f"WARNING: Could not create symlink: {e}")
    else:
        print(f"WARNING: full_world_path specified but does not exist: {full_world_path}")