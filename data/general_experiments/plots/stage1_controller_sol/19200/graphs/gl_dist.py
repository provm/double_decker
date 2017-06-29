#!/usr/bin/python

import collections

f = open("../log", "r")
fw = open("../plot.dat", "w")

content = f.read()
glob = {}
soft = {}
start_seconds = ''
mx = 0
ln = ''

def page_to_mb(pages):
    return round(pages/256.0, 2)

def time_to_seconds(t):
    return sum(int(x) * 60 ** i for i,x in enumerate(reversed(t.split(":"))))

def relative_seconds(t):
    return time_to_seconds(t) - start_seconds

start_seconds = time_to_seconds(content[7:16])

for line in content.split("\n"):
    if 'Global Reclaim-' in line:        
        if 'Global Reclaim-0' not in line:
            if line[7:16] in glob:
                glob[line[7:16]] += int(line[63:])
            else:
                glob[line[7:16]] = int(line[63:])
    
    elif 'Soft Reclaim-' in line:        
        if 'Soft Reclaim-0' not in line:
            
            if mx < int(line[61:]):
                mx = int(line[61:])
                ln = line
            
            if line[7:16] in soft:
                soft[line[7:16]] += int(line[61:])
            else:
                soft[line[7:16]] = int(line[61:])
            
glob_od = collections.OrderedDict(sorted(glob.items()))
glob = collections.OrderedDict()

soft_od = collections.OrderedDict(sorted(soft.items()))
soft = collections.OrderedDict()

#print "GLOBAL"   
for g in glob_od:
    #print g, "\t", relative_seconds(g), "\t", glob_od[g], "\t", glob_od[g] 
    glob[relative_seconds(g)] = glob_od[g]

#print "SOFT"        
for g in soft_od:
    #print g, "\t", relative_seconds(g), "\t", soft_od[g], "\t", soft_od[g] 
    soft[relative_seconds(g)] = soft_od[g]
        

tot_s = 0
tot_g = 0
    
#print "OUTPUT"
for i in range(320):
    
    if i in soft:
        tot_s = soft[i]
    
    if i in glob:
        tot_g = glob[i]
    
    line = str(i)+"\t"+str(page_to_mb(tot_s))+"\t"+str(page_to_mb(tot_g))
     
    #print(line)   
    fw.write(line+"\n")  

print "Max reclaim:", mx, mx/256.0

