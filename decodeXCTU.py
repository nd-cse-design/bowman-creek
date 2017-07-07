import sys
import os

'''
Save log file by clicking save button in XCTU
Run this program: python decodeXCTU.py logfilename outputfile
'''
filename = sys.argv[1]
newfilename = sys.argv[2]
newfile = open(newfilename, 'w')

with open(filename) as f:
	for line in f:
		line = line.strip()
		newfile.write(bytes.fromhex(line).decode('utf-8'))
		# print(bytes.fromhex(line).decode('utf-8'))

	# Cleaning up the files
	f.close()
	newfile.close()