import numpy as np

data = np.loadtxt("../collected.dat",skiprows=0)
#data = data/1048576

fw = open("../plot.dat", "w")


for i in range(len(data[:,8])):
    
    exceed1 = (data[i,1]-data[i,2])
    exceed2 = (data[i,4]-data[i,5])
    exceed3 = (data[i,7]-data[i,8])
    exceed4 = (data[i,10]-data[i,11])
    #exceed5 = data[i,13]#(data[i,13]-data[i,14])
 
    line = str(i) + "\t" + \
        str(exceed1) + "\t"+ \
        str(exceed2) + "\t"+ \
        str(exceed3) + "\t"+ \
        str(exceed4) + "\t"+ \
        str(data[i, 13]) + "\n"
            
    fw.write(line)

