"""
@author: Xueyang Liu
"""
import re
import math
import random
import sys
import time
import traceback
import argparse

############## Utility function for memory request parsing ############
def footprint_addr(addr, size):
    # convert input address and size to a output set of all memory address that's covered(assume byte addressable memory)
    # addr(int): memory address
    # size(int): the memory access size
    # output(set): a set of all memory addresses that's covered
    output = set()
    for i in range(size):
        output.add(addr+i)
    return output



################ Parse input arguments ####################
parser = argparse.ArgumentParser(description='Trace analyzer, including memory trasaction amount and footprint size.')
parser.add_argument('--trace', type=str, required=True,
                    help='Input trace file to pass. Has to be the same format as pinatrace tool output')
parser.add_argument('--out', type=str, help='Analyzer rofile output result file name')
args = parser.parse_args()
inputTrace = args.trace
if args.out:
    outputfilename = args.out
else:
    outputfilename = inputTrace + ".memProfile"
trace_file_writer=open(outputfilename,'w') 							#open a write connection to the output file
try:
    read_amount = 0
    write_amount = 0
    read_addr_range = set()
    write_addr_range = set()
    with open(inputTrace,'r') as ff:
        for count, line in enumerate(ff):
            textlist = re.split(' +', line)
            # print(textlist)
            if len(textlist) != 5:
                # print(line)
                continue
            opcode = textlist[1]
            address = int(textlist[2],16)
            size = int(textlist[3])
            # new_footprint = footprint_addr(address, size)
            if opcode == "R":
                read_amount = read_amount + size
                # read_addr_range = read_addr_range.union(new_footprint)
                for i in range(size):
                    read_addr_range.add(address+i)
            elif opcode == "W":
                write_amount = write_amount + size
                # write_addr_range = write_addr_range.union(new_footprint)
                for i in range(size):
                    write_addr_range.add(address+i)
            else:
                print("[Warning!!] Invalid opcode in line: "+line)
                continue
except IOError:
    print('Cannot find the trace file, please check it.')
    sys.exit()

################# Print and save result ##################
result_str = "Trace analysis result for: " + inputTrace + "\n" + \
                "----------------------------------------------------------------\n" + \
                "Total amount of read transactions:  {:.2f} KB\n".format(read_amount/1024) + \
                "Total amount of write transactions: {:.2f} KB\n".format(write_amount/1024) + \
                "Memory read footprint size:         {:.2f} KB\n".format(len(read_addr_range)/1024) + \
                "Memory write footprint size:        {:.2f} KB\n".format(len(write_addr_range)/1024) + \
                "Read transaction/footprint ratio:   {:.3f}\n".format(read_amount/len(read_addr_range)) + \
                "Write transaction/footprint ratio:  {:.3f}\n".format(write_amount/len(write_addr_range))
print(result_str)
trace_file_writer.write(result_str)
trace_file_writer.close()