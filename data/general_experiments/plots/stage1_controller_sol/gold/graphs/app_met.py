import re

f = open('../m1.txt')
fw = open('../app_met.dat','w')
content = f.read()
count = content.count('operations;') - 1
#print count
time = []

for i in range(count):
        time.append(str((i+1)*10))

files = ['../m1.txt', '../m2.txt', '../r1.txt', '../r2.txt'] 

w, h = 4, count
throughput = [[0 for x in range(w)] for y in range(h)] 
#print len(time)
i = -1


for fname in files:
        
        i += 1
        j = -1
        
        with open(fname) as f:
            content = f.readlines()
            
        for line in content:
                
                words = line.split(";")
                
                if len(words) > 2 and j<count:
                        j += 1
                        #print (i, j)
                        #print line
                        tr = re.findall("\d+.\d*", words[1])
                        try:
                                throughput[j][i] = tr[0]
                        except:
                                try:
                                        throughput[j][i] = "0"
                                except:
                                        pass

for i in range(count):
        
        #print type(throughput[i][2]), throughput[i][2]

        line = time[i] + \
                "\t" + str(throughput[i][0]) + \
                "\t" + str(throughput[i][1]) + \
                "\t" + str(throughput[i][2]) + \
                "\t" + str(throughput[i][3])
        fw.write(line+"\n")                
        
