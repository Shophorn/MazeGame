import os

# -V means outputting spir-v intermediate binary code
os.system("glslangValidator -V src/shaders/shader.vert -o shaders/vert.spv")
os.system("glslangValidator -V src/shaders/shader.frag -o shaders/frag.spv")

os.system("glslangValidator -V src/shaders/shadow_shader.vert -o shaders/shadow_vert.spv")
os.system("glslangValidator -V src/shaders/shadow_view.frag -o shaders/shadow_view_frag.spv")

os.system("glslangValidator -V src/shaders/line_shader.vert -o shaders/line_vert.spv")
os.system("glslangValidator -V src/shaders/line_shader.frag -o shaders/line_frag.spv")

os.system("glslangValidator -V src/shaders/sky_shader.vert -o shaders/vert_sky.spv")
os.system("glslangValidator -V src/shaders/sky_shader.frag -o shaders/frag_sky.spv")

os.system("glslangValidator -V src/shaders/gui_shader2.vert -o shaders/gui_vert2.spv")
os.system("glslangValidator -V src/shaders/gui_shader2.frag -o shaders/gui_frag2.spv")

