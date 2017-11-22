from os import listdir
from os.path import isfile, join
import glob
import re

# onlyfiles = [f for f in listdir("./") if isfile(join("./", f))]
# print(*onlyfiles, sep='\n')
def getint(name):
    basename = name.partition('.')[0]
    num = re.sub('mix', '', basename.split('-')[0])
    return int(num)

files = glob.glob('*.txt')
files.sort(key=getint)

print(files)
