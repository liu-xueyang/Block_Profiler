# -*- coding: utf-8 -*-
"""
Created on SAT Mar 5 2016
@author: Suju, Tian, Yao
"""
import re
import math
import random
import sys
import time
import configparser
import traceback
import argparse

################ Parse input arguments ####################
parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('--trace', type=str, required=True,
                    help='Input trace file to pass. Has to be the same format as pinatrace tool output')
parser.add_argument('--out', type=str, help='Profile output result file name')
parser.add_argument('--traceout', type=str, help='L2 access trace file name')
args = parser.parse_args()
workload = args.trace
if args.out:
    outputfilename = args.out
else:
    outputfilename = workload + ".profile"
if args.traceout:
    outputTraceName = args.traceout
else:
    outputTraceName = workload + ".L2"
trace_file_writer=open(outputTraceName,'w') 							#open a write connection to the output file

################ cache_conf reader : start ################ 
print('Loading the configuration file...')
config = configparser.ConfigParser()
try:
    config.read("cache_conf.ini")
    hitTimeL1 = config.getint("DL1_core","hitDelay")
    misspenalty = config.getint("DL1_core","missPenalty")
    cacheSize = eval(config.get("DL1_core","size"),{"__builtins__":None},{})
    cacheBlockSize = eval(config.get("DL1_core","bsize"),{"__builtins__":None},{})
    way = eval(config.get("DL1_core","assoc"),{"__builtins__":None},{})
    policy = config.get("DL1_core","replPolicy")
    # workload = config.get("WorkLoad","workLoad")

except configparser.NoSectionError:
	print('The cache_conf.ini file is missing! Please check the file.')
	time.sleep(1)
	sys.exit()

def policySelector():
    policyNumber = 3
    if re.search('random', policy, re.IGNORECASE) :
        policyNumber = 0
    if re.search('lru', policy, re.IGNORECASE) :
        policyNumber = 1
    if re.search('fifo', policy, re.IGNORECASE) :
        policyNumber = 2
    if policyNumber == 3:
        print('can not recognize the replacement policy, please check the config file.')
        time.sleep(1)
        sys.exit()
    return policyNumber

policyNumber = policySelector()
################ cache_conf reader : end  ################ 
print('Configuration Check...Done')

################ Cache initialize : start  ################
indexSize = int(cacheSize/(cacheBlockSize*way))
bitsOfIndex = int(math.log(cacheSize/(cacheBlockSize*way))/math.log(2))
# print bitsOfIndex
offSet = int(math.log(cacheBlockSize)/math.log(2))
# print offSet
bitsOfTag = 64 - bitsOfIndex - offSet   #32bit and 64bit

programTimer = 0
process = 0
hit=0
miss=0
linenum = 0
p=0
dot='.'
try:
    with open(workload,'r') as ff:
        for count, line in enumerate(ff):
            if count % 300000 == 0 and count >= 300000:
                p+=1
                sys.stdout.write('Task size assessing'+'%'*p+dot*(10-p)+'\r')
                sys.stdout.flush()
                if p >= 10:
                    p=0
            pass
        step = (count+1)/100
        print('Task size assessing..........Finished')
except IOError:
    print('Cannot find the trace file, please check it.')
    sys.exit()
print('Initialize the timer and counter...Done')
cacheTable = [ [ 0 for i in range(way) ] for j in range(indexSize) ]    #original Cache Table set up
validTable = [ [ 0 for i in range(way) ] for j in range(indexSize) ]    #set the valid bit for Cache
timeTable  = [ [ 0 for i in range(way) ] for j in range(indexSize) ]    #record the access time of each set
print('Call the space for Cache Simulator...Done')
################ Cache initialize : end  ################

try:
    with open(workload) as f:
        for line in f:
            # text = line[35:46]
            textlist = re.split(' +', line)
            # print(textlist)
            if len(textlist) != 5:
                print(line)
                continue
            text = textlist[2]
            linenum +=1
            if  linenum%step==0 and linenum >= step :
                process += 1
                sys.stdout.write('Simulator running...'+str(process)+'%\r')
                sys.stdout.flush()
            if text == '0':
                continue
            miss_flag=1
            # text = '0x'+text
            tag = int(text,16) >> (64 - bitsOfTag)  #32bit and 64bit      
            st = str(bin(int(text,16)))
            st = st[-offSet-bitsOfIndex:-offSet]
            index = int(st,2)

            if index<0:
                print('Index = ',index)  #sanity check 1
            if tag<0: 
                print('Index = ',tag)    #sanity check 2

            ######################## Hit part ########################
            for i in range(way):
                if cacheTable[index][i] == tag:
                    hit+=1
                    if policyNumber == 1:
                        timeTable[index][i]=programTimer            #update the timestamp in the cache hit
                                                                    #only use this in the LRU algorithm.
                    miss_flag=0
                    break
            ######################## Miss part ########################    
            # trace_file_writer.write(line)    
            if miss_flag == 1:
                trace_file_writer.write(line)
                if policyNumber == 1 or policyNumber == 2:              # FIFO & LRU Replacement Algorithm
                    for j in range(way):
                        if(validTable==0):                          #find the empty row to replace
                            cacheTable[index][j]=tag
                            timeTable[index][j]=programTimer
                            validTable[index][j]=1
                            miss_flag=0
                            miss+=1
                            break
                    min=timeTable[index][0]                         #find the oldest one
                    min_k=0
                    for k in range(way):
                        # if k == 0:         
                        if timeTable[index][k]<min :
                            min=timeTable[index][k]
                            min_k=k
                        
                    cacheTable[index][min_k]=tag                    #replace the oldest one(for LRU)
                    timeTable[index][min_k]=programTimer            #replace the first one(for FIFO)  
                    miss += 1
                    miss_flag = 0
                elif policyNumber == 0:                                 #Random Replacement Algorithm
                    for j in range(way):
                        if(validTable==0):                          #find the empty row to replace
                            cacheTable[index][j]=tag
                            validTable[index][j]=1
                            miss_flag=0
                            miss+=1
                            break
                    randomWay = random.randint(0, way-1)            #find a random one
                    cacheTable[index][randomWay]=tag                #replace with the random one  
                    miss += 1
                    miss_flag = 0
            programTimer += 1                                       #Timer for the whole simulator process
            

except Exception:
    # print 'Cannot find the trace file, please check it.'
    # print linenum
    traceback.print_exc()
    sys.exit()

trace_file_writer.close()
sys.stdout.write('Simulator...Done!            \n')
missrate=float(miss)/(hit+miss)*100.0
averageMemoryAccessTime = hitTimeL1 + missrate/100*misspenalty
print('------------- Result -------------')
print('AMAT: ', round(averageMemoryAccessTime,2),'(Cycles)')
print('Cache access: ', hit+miss)
print('Cache hit   : ', hit)
print('Cache misses: ',miss)
print('Missrate    : ',round(missrate,4),'%')
print('----------------------------------')
print('readline:',linenum)
# Write in result file
print('final result file output:'+ outputfilename)
file_writer=open(outputfilename,'w') 							#open a write connection to the output file
file_writer.write('Workload\t: ' + workload +'\n') 
file_writer.write('----------------------------------------------------------------\n')
file_writer.write('Cache Size\t: ' + str(cacheSize) +'\t\t'+'Cache Block Size\t: ' + str(cacheBlockSize) +'\n') #write
file_writer.write('Associativity\t: ' + str(way) + '\t\t'+ 'Replacement Policie\t: ' + policy+'\n')
file_writer.write('----------------------------------------------------------------\n')
file_writer.write('AMAT(cycles)\t: ' + str(round(averageMemoryAccessTime,2)) +'\n')
file_writer.write('----------------------------------------------------------------\n')
file_writer.write('Cache Access\t: ' + str(hit+miss) +'\n')
file_writer.write('Cache Miss\t: ' + str(miss) + '\t\t'+'Cache Hit\t: ' + str(hit)+'\n')
file_writer.write('Missrate\t: ' + str(round(missrate,4)) +'%'+'\n')
file_writer.close()
