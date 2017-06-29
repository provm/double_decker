import sys
import re

s=0.0
n=0

#print sys.argv[1]

with open(sys.argv[1]) as f:
	for line in f:
		if "MEM ONLY  :" in line:
			s += int(re.findall('\d+', line)[0])
			n += 1
			#print s, n

print s/n
