import glob
import subprocess
import os


# Folders with cpp files
SRC_FOLDER = [
    ".",
    "upgrades"
]

BUILD_FOLDER = "build"

patterns = [f"{x}/*.cpp" for x in SRC_FOLDER]
cpp_files = []

for p in patterns:
    cpp_files.extend(glob.glob(p))

# filename without the directory or its extension
base_filename = [os.path.splitext(os.path.basename(x))[0] for x in cpp_files]


# Compile to object files
obj_files = [f"{BUILD_FOLDER}/{x}.o" for x in base_filename]
for i, f in enumerate(cpp_files):
    cmd = ["g++", f, "-c", "-o", obj_files[i]]
    print(' '.join(cmd))
    subprocess.run(cmd)

# Final compile
compile_command = ["g++", "-std=c++11"]
compile_command.extend(obj_files)
compile_command.extend(["-o", "main.exe"])
print(' '.join(compile_command))

subprocess.run(compile_command)
