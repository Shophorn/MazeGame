import os

# -V means outputting spir-v intermediate binary code

targets = [
	("shader.vert", 			"vert.spv"),
	("shader.frag", 			"frag.spv"),
	("terrain_shader.frag", 	"terrain_frag.spv"),
	("animated_shader.vert", 	"animated_vert.spv"),

	("shadow_shader.vert", 		"shadow_vert.spv"),
	("shadow_view.frag", 		"shadow_view_frag.spv"),

	("line_shader.vert", 		"line_vert.spv"),
	("line_shader2.vert", 		"line_vert2.spv"),
	("line_shader.frag", 		"line_frag.spv"),

	("sky_shader.vert", 		"vert_sky.spv"),
	("sky_shader.frag", 		"frag_sky.spv"),

	("gui_shader2.vert", 		"gui_vert2.spv"),
	("gui_shader3.vert", 		"gui_vert3.spv"),
	("gui_shader2.frag", 		"gui_frag2.spv"),

	("leaves.vert", 			"leaves_vert.spv"),
	("leaves.frag", 			"leaves_frag.spv"),
	("leaves_shadow.vert", 		"leaves_shadow_vert.spv"),
	("leaves_shadow.frag", 		"leaves_shadow_frag.spv"),

	("triplanar.frag",			"triplanar_frag.spv")
]

for target in targets:
	os.system("glslangValidator -V src/shaders/{} -o assets/shaders/{}".format(target[0], target[1]))