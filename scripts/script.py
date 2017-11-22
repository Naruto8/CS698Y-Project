import re
import sys

filename = sys.argv[1]

LLC_load_misses = []
L2C_load_misses = []
L1D_load_misses = []
L1I_load_misses = []
CPU_instr = []
for line in open(filename, 'r'):
    if re.search("LLC LOAD", line):
        l = line.split()
        LLC_load_misses.append(l[-1])
        if line == None:
            print("Error in finding LLC LOADS")

    if re.search("L2C LOAD", line):
        l = line.split()
        L2C_load_misses.append(l[-1])
        if line == None:
            print("Error in finding L2C LOADS")

    if re.search("L1D LOAD", line):
        l = line.split()
        L1D_load_misses.append(l[-1])
        if line == None:
            print("Error in finding L1D LOADS")

    if re.search("L1I LOAD", line):
        l = line.split()
        L1I_load_misses.append(l[-1])
        if line == None:
            print("Error in finding L1I LOADS")

    if re.search(r'CPU . cumulative', line):
        l = line.split()
        CPU_instr.append(l[-3])
        if line == None:
            print("Error in finding CPU CUMULATIVE line")

LLC_load_misses = list(map(int, LLC_load_misses))
L2C_load_misses = list(map(int, L2C_load_misses))
L1D_load_misses = list(map(int, L1D_load_misses))
L1I_load_misses = list(map(int, L1I_load_misses))
CPU_instr = list(map(int, CPU_instr))
llc_load_mpki = [(llc/instr)*1000 for llc, instr in zip(LLC_load_misses, CPU_instr)]
l2c_load_mpki = [(l2c/instr)*1000 for l2c, instr in zip(L2C_load_misses, CPU_instr)]
l1d_load_mpki = [(l1d/instr)*1000 for l1d, instr in zip(L1D_load_misses, CPU_instr)]
l1i_load_mpki = [(l1i/instr)*1000 for l1i, instr in zip(L1I_load_misses, CPU_instr)]
print(llc_load_mpki)
print(l2c_load_mpki)
print(l1d_load_mpki)
print(l1i_load_mpki)
