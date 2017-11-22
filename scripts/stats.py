import re
import os
import glob

def getint(name):
    basename = name.partition('.')[0]
    num = re.sub('mix', '', basename.split('-')[0])
    return int(num)

files = glob.glob('*.txt')
files.sort(key=getint)

instr_pattern = re.compile(r'CPU . cumulative')
loadpattern = re.compile("LOAD")
# path = '/data/ndesh/CS698Y/assignment1/'
policies = ['LRU', 'Hawkeye', 'SHiP']#, 'hawkeye2', 'hawkeye']
caches = ['LLC', 'L2C', 'L1I', 'L1D']
cores = ['CPU0', 'CPU1', 'CPU2', 'CPU3']
apps = ['bc', 'bfs', 'cc', 'pr', 'sssp', 'tc']
inputs = [['twitter', 'kron'], ['web', 'urand']]

LLC_load_misses = []
L2C_load_misses = []
L1D_load_misses = []
L1I_load_misses = []
CPU_instr = []

# MPKI
for f in files: 
    count = 0
    c = 0
    misses = 0
    for i, line in enumerate(open(f)):
        if re.search(r'CPU . cumulative', line):
            c += 1
            l = line.split()
            if c > 4:
                CPU_instr.append(int(l[-3]))
            if line == None:
                print("Error in finding CPU CUMULATIVE line")
        if re.search(loadpattern,line):
            count += 1
            if count > 16:
                if count%4==0:
                    LLC_load_misses.append(int(line.split()[-1]))
                elif count%4==1:
                    L1D_load_misses.append(int(line.split()[-1]))
                elif count%4==2:
                    L1I_load_misses.append(int(line.split()[-1]))
                elif count%4==3:
                    L2C_load_misses.append(int(line.split()[-1]))

llc_load_mpki = [round((llc/instr)*1000,3) for llc, instr in zip(LLC_load_misses, CPU_instr)]
l2c_load_mpki = [round((l2c/instr)*1000,3) for l2c, instr in zip(L2C_load_misses, CPU_instr)]
l1i_load_mpki = [round((l1i/instr)*1000,3) for l1i, instr in zip(L1I_load_misses, CPU_instr)]
l1d_load_mpki = [round((l1d/instr)*1000,3) for l1d, instr in zip(L1D_load_misses, CPU_instr)]

mpkis = [llc_load_mpki, l2c_load_mpki, l1i_load_mpki, l1d_load_mpki]

for r in range(2):
    for l, cache in enumerate(caches):
        outfile = open("mpki_stats/" + cache + "_MPKI_" + str(r) + ".csv", 'w')
        outfile.write("Application, LRU, Hawkeye, SHiP\n")
        for c, core in enumerate(cores):
            for a, app in enumerate(apps):
                outfile.write(app + "-" + inputs[r][1 if app=="tc" else 0] + "-" + core + ",")
                for p, policy in enumerate(policies):
                    outfile.write(str(mpkis[l][a*12+p*4+c]) + ("\n" if p==2 else ","))                
