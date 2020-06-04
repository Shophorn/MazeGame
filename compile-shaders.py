import os

# -V means outputting spir-v intermediate binary code
os.system("glslangValidator -V src/shaders/shader.vert -o assets/shaders/vert.spv")
os.system("glslangValidator -V src/shaders/shader.frag -o assets/shaders/frag.spv")
os.system("glslangValidator -V src/shaders/terrain_shader.frag -o assets/shaders/terrain_frag.spv")
os.system("glslangValidator -V src/shaders/animated_shader.vert -o assets/shaders/animated_vert.spv")

os.system("glslangValidator -V src/shaders/shadow_shader.vert -o assets/shaders/shadow_vert.spv")
os.system("glslangValidator -V src/shaders/shadow_view.frag -o assets/shaders/shadow_view_frag.spv")

os.system("glslangValidator -V src/shaders/line_shader.vert -o assets/shaders/line_vert.spv")
os.system("glslangValidator -V src/shaders/line_shader2.vert -o assets/shaders/line_vert2.spv")
os.system("glslangValidator -V src/shaders/line_shader.frag -o assets/shaders/line_frag.spv")

os.system("glslangValidator -V src/shaders/sky_shader.vert -o assets/shaders/vert_sky.spv")
os.system("glslangValidator -V src/shaders/sky_shader.frag -o assets/shaders/frag_sky.spv")

os.system("glslangValidator -V src/shaders/gui_shader2.vert -o assets/shaders/gui_vert2.spv")
os.system("glslangValidator -V src/shaders/gui_shader3.vert -o assets/shaders/gui_vert3.spv")
os.system("glslangValidator -V src/shaders/gui_shader2.frag -o assets/shaders/gui_frag2.spv")

targets = [
	("leaves.vert", "leaves_vert.spv"),
	("leaves.frag", "leaves_frag.spv"),
	("leaves_shadow.vert", "leaves_shadow_vert.spv"),
	# ("leaves_shadow.frag", "leaves_shadow_frag.spv")
]

for target in targets:
	os.system("glslangValidator -V src/shaders/{} -o assets/shaders/{}".format(target[0], target[1]))