# Build file for Vulkan tutorial
# vulkan-tutorial.com

def SetArgument(switch, arguments):
	if switch is None:
		switch = ''

	if len(arguments) == 0:
		return ''

	# Add leading space here
	result = ' ' + ' '.join([switch + item for item in arguments])
	return result

import subprocess
import os
import sys

silent = '--silent' in sys.argv

# Build settings
include = [
	"include", 
	"C:/VulkanSDK/1.1.108.0/Include"
]

lib_path = [
	"lib",
	"C:/VulkanSDK/1.1.108.0/Lib"
]

lib_names = [
	"libglfw3",
	"vulkan-1",
	"gdi32"
]

sources = ['src/main.cpp']
outputName = 'mazegame.exe'
flags = ["-std=c++17"]

# call = CompileClangPlusPlusCall()
include_str = SetArgument('-I', include)
libPath_str = SetArgument('-L', lib_path)
link_str 	= SetArgument('-l', lib_names)
sources_str = SetArgument(None, sources)
flags_str 	= SetArgument(None, flags)

call = "clang++ {} {} -o {} {} {} {}".format(
			flags_str,
			include_str,
			outputName,
			sources_str,
			libPath_str,
			link_str)

if not silent:
	print (call)

output_file_name = "vkTutorial.build_output"

result = subprocess.run(call)

if not silent:
	if result.returncode == 0:
		print ("Great SUCCESS")
		# print ('\033[92m' + "Great SUCCESS" + '\033[0m')
	else:
		# os.system("editor file " + output_file_name)
		print ("Did NOT WORK")

exit(result.returncode)

