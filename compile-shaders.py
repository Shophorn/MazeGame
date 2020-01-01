import os

# -V means outputting spir-v intermediate binary code
os.system("glslangValidator -V src/shader.vert -o shaders/vert.spv")
os.system("glslangValidator -V src/shader.frag -o shaders/frag.spv")
os.system("glslangValidator -V src/shader2.frag -o shaders/frag2.spv")

os.system("glslangValidator -V src/sky_shader.vert -o shaders/vert_sky.spv")
os.system("glslangValidator -V src/sky_shader.frag -o shaders/frag_sky.spv")

os.system("glslangValidator -V src/gui_shader.vert -o shaders/vert_gui.spv")
os.system("glslangValidator -V src/gui_shader.frag -o shaders/frag_gui.spv")