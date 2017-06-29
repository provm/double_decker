import numpy as np

data = np.loadtxt("../collected.dat",skiprows=0)
#data = data/1048576

fw = open("../plot.dat", "w")

olda1 = 0
olda2 = 0
olda3 = 0
olda4 = 0

for i in range(len(data[:,1])):
    line = str(i)+"\t" \
                + str(data[i,3]-olda1) + "\t" \
                + str(data[i,6]-olda2) + "\t" \
                + str(data[i,9]-olda3) + "\t" \
                + str(data[i,12]-olda4) + "\n" 
    fw.write(line)
    olda1 = data[i, 3]
    olda2 = data[i, 6]
    olda3 = data[i, 9]
    olda4 = data[i, 12]

