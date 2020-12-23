targets = [
	("shader.vert", 			"vert.spv"),
	("shader.frag", 			"frag.spv"),
	("animated_shader.vert", 	"animated_vert.spv"),
	("triplanar.frag",			"triplanar_frag.spv"),

	("shadow_shader.vert", 		"shadow_vert.spv"),

	("line_shader.vert", 		"line_vert.spv"),
	("line_shader.frag", 		"line_frag.spv"),

	("sky_shader.vert", 		"vert_sky.spv"),
	("sky_shader.frag", 		"frag_sky.spv"),

	("gui_shader.vert", 		"gui_vert.spv"),
	("gui_shader.frag", 		"gui_frag.spv"),

	("leaves.vert", 			"leaves_vert.spv"),
	("leaves.frag", 			"leaves_frag.spv"),
	("leaves_shadow.vert", 		"leaves_shadow_vert.spv"),
	("leaves_shadow.frag", 		"leaves_shadow_frag.spv"),

	("hdr.frag",				"hdr_frag.spv"),
	("hdr.vert",				"hdr_vert.spv"),
]

import os
import sys

def compile(output_folder):
	# -V means outputting spir-v intermediate binary code
	for target in targets:
		os.system("glslangValidator -V src/shaders/{} -o {}/{}".format(target[0], output_folder, target[1]))

if __name__ == '__main__':
	output_folder = "development/shaders"
	if len(sys.argv) > 1:
		output_folder = sys.argv[1]
	compile(output_folder)