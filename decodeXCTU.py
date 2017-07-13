import sys
import os

'''
Save log file by clicking save button in XCTU
Save it to directory the script is in
Run this program: python decodeXCTU.py logfilename outputfile
'''

filename = sys.argv[1]
newfilename = sys.argv[2]
newfile = open(newfilename, 'w')

with open(filename, "r") as f:
	next(f)
	next(f)
	for line in f:
		line = line.strip().split(",")
		if line[2] == 'RECV':
			newfile.write(bytes.fromhex(line[3]).decode('utf-8'))

	# Cleaning up the files
	f.close()
	newfile.close()