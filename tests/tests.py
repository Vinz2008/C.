from termcolor import colored
import subprocess
import os

nb_test = 1

def test_file(name):
	global nb_test
	cmd = "../cpoint -std ../std -c " + name
	print("CMD " + cmd)
	return_val = subprocess.call(cmd, shell=True, stdout=open(os.devnull, 'wb'))
	print("RETURN VALUE : " + str(return_val))
	if (return_val == 1):
		print(colored("TEST " + str(nb_test) + " FAILED", "red"))
		exit(1)
	nb_test += 1

for i in range(1,14):
	test_file("../test" + str(i) + ".cpoint")
print(colored("All " + str(nb_test) + " tests succeeded", "green"))