import re
import os

pattern = re.compile("LLC TOTAL")
# path = '/data/ndesh/CS698Y/assignment1/'
policies = ['lru']#, 'hawkeye2', 'hawkeye']
caches = ['L1D', 'L1I', 'L2C', 'LLC']
apps = ['cassandra', 'classification', 'cloud9', 'nutch', 'streaming']

# MPKI
for cache in caches:
    pattern = re.compile(cache + " RFO")
    outfile = open('cloud_stats/MPKI/' + cache + ".csv", 'w')
    outfile.write("Applicaiton,LRU,Hawkeye Modified,Hawkeye\n")
    for app in apps: 
        for phase in range(6):
            for policy in policies:
                ignore = 0
                misses = 0
                for i, line in enumerate(open('results_100M/' + app+"_phase"+str(phase) + '-bimodal-no-no-' + policy + '-4core.txt')):
                    for match in re.finditer(pattern, line):
                        ignore += 1
                        if ignore < 5:
                            a = [int(s) for s in line.split() if s.isdigit()]
                            mpki = a[2]*1000.0/100000000
                            outfile.write(app+"_phase"+str(phase))
                            outfile.write("_core%d,%f\n" % (ignore-1, mpki))
            # outfile.write("\n")

# APKI
for cache in caches:
    pattern = re.compile(cache + " TOTAL")
    outfile = open('cloud_stats/APKI/' + cache + ".csv", 'w')
    outfile.write("Applicaiton,LRU,Hawkeye Modified,Hawkeye\n")
    for app in apps: 
        for phase in range(6):
            outfile.write(app+"_phase"+str(phase))
            for policy in policies:
                ignore = 0
                access = 0
                for i, line in enumerate(open('results_100M/' + app+"_phase"+str(phase) + '-bimodal-no-no-' + policy + '-4core.txt')):
                    for match in re.finditer(pattern, line):
                        ignore += 1
                        if ignore < 5:
                            a = [int(s) for s in line.split() if s.isdigit()]
                            access += a[0]
                apki = access*1000.0/400000000
                outfile.write(",%f" % apki) 
            outfile.write("\n")

# MR 
for cache in caches:
    pattern = re.compile(cache + " TOTAL")
    outfile = open('cloud_stats/MR/' + cache + ".csv", 'w')
    outfile.write("Applicaiton,LRU,Hawkeye Modified,Hawkeye\n")
    for app in apps: 
        for phase in range(6):
            outfile.write(app+"_phase"+str(phase))
            for policy in policies:
                ignore = 0
                misses = 0
                access = 0
                for i, line in enumerate(open('results_100M/' + app+"_phase"+str(phase) + '-bimodal-no-no-' + policy + '-4core.txt')):
                    for match in re.finditer(pattern, line):
                        ignore += 1
                        if ignore < 5:
                            a = [int(s) for s in line.split() if s.isdigit()]
                            access += a[0]
                            misses += a[2]
                mr = misses*100.0/access
                outfile.write(",%f" % mr) 
            outfile.write("\n")
