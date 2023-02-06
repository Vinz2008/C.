from termcolor import colored
import subprocess
import os
import sys

nb_test = 0

def test_file(name, verbose=True):
	global nb_test
	cmd = "../cpoint -std ../std -c " + name
	print("CMD " + cmd)
	return_val = 0
	if verbose == False:
		fd = open(os.devnull, 'wb')
		return_val = subprocess.call(cmd, shell=True, stdout=fd)
	else:
		return_val = subprocess.call(cmd, shell=True)
	print("RETURN VALUE : " + str(return_val))
	if (return_val == 1):
		print(colored("TEST " + str(nb_test) + " FAILED", "red"))
		exit(1)
	nb_test += 1

verbose = False
for i, arg in enumerate(sys.argv):
    #print(f"Argument {i:>6}: {arg}")
	if arg == "-v":
		verbose = True

for i in range(0,19):
	print(i+1)
	test_file("./test" + str(i+1) + ".cpoint", verbose)
print(colored("All " + str(nb_test) + " tests succeeded", "green"))