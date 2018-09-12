import sys

print(sys.argv)
with open("keys.logs", "w") as fp:
	for arg in sys.argv:
		fp.write(arg + " ")
	fp.write("\n")