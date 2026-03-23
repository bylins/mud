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

print(f"Setting up world data in {build_dir}...")

lib_src = os.path.join(source_root, "lib")
template_src = os.path.join(source_root, "lib.template")

if os.path.exists(small_world_dir):
    shutil.rmtree(small_world_dir)

if os.path.exists(lib_src):
    shutil.copytree(lib_src, small_world_dir)
else:
    print(f"ERROR: Source 'lib' not found at {lib_src}")
    sys.exit(1)

if os.path.exists(template_src):
    for item in os.listdir(template_src):
        s = os.path.join(template_src, item)
        d = os.path.join(small_world_dir, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, dirs_exist_ok=True)
        else:
            shutil.copy2(s, d)
else:
    print(f"Warning: 'lib.template' not found at {template_src}")

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