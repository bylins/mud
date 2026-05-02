import sys
import shutil
import os

def copy_data(src, dst, stamp_file):
    dst_dir = os.path.dirname(dst)
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
    
    if os.path.isdir(src):
        if os.path.exists(dst):
            shutil.rmtree(dst)
        shutil.copytree(src, dst)
    else:
        shutil.copy2(src, dst)

    with open(stamp_file, 'w') as f:
        os.utime(stamp_file, None)

if __name__ == '__main__':
    copy_data(sys.argv[1], sys.argv[2], sys.argv[3])