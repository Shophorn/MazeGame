from ctypes import *
dll = CDLL("C:/Users/Leo/Work/friendsimulator/asset_cooker_for_python.dll")
dll.cook_mesh.argtypes = [c_char_p, c_char_p, c_char_p]

dll.cook_mesh(c_char_p("Hello"), "Again", "B")