import sys
import re

s=0.0
n=0

#print sys.argv[1]

with open(sys.argv[1]) as f:
	for line in f:
		digits = re.findall('\d+', line)
		#print digits
		
		if len(digits)==6:
			s += int(digits[5])
			n += 1
			#print s, n

print (s/n)/256
