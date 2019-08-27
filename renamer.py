import os
import re

def execute():

	pattern = "win_(.*)"

	for filename in os.listdir("src/"):
		
		match = re.findall(pattern, filename)
		if match:
			
			newfilename = "winapi_" + match[0]
			print (newfilename)
			os.rename("src/" + filename, "src/" + newfilename)

if __name__ == '__main__':
	execute()