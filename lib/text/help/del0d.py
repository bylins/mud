import glob, os
files = glob.glob("*.*")
for file in files:
    if not os.path.isfile(file):
	    continue
    fp = open(file, "r", encoding="koi8-r").read()
    fp = fp.replace("\n\r", "\n")
    fc = open(file, "w", encoding="koi8-r")
    fc.write(fp)
    fc.close()
