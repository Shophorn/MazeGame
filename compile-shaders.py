import os

# -V means outputting spir-v intermediate binary code
os.system("glslangValidator -V src/shader.vert -o shaders/vert.spv")
os.system("glslangValidator -V src/shader.frag -o shaders/frag.spv")