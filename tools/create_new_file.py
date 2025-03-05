import sys
import os

NAMESPACE_NAME = 'mk'
CMAKE_FILE_PATH = './CMakeLists.txt'
SRC_PATH = 'src/'
INCLUDE_PATH = 'include/'
SUB_FOLDER = "Game"

DEFAULT_FILE_TEMPLATE = (
# Header file
'''
#pragma once

namespace {0}::{1} 
{{

}}   
''',
# Source file
'''
#include \"''' + SUB_FOLDER + '''/{2}/{1}.h\"

namespace {0}::{1} 
{{

}}  
''')

FILE_TEMPLATES = {}

if __name__ == '__main__':
    file_type = sys.argv[1]
    file_name = sys.argv[2]

    file_type_name = file_type.capitalize()

    # Take the file_type, create the header and source file, then add the source file to the CMakeLists.txt
    header_file = os.path.join(INCLUDE_PATH, SUB_FOLDER, file_type_name, file_name + '.h')
    source_file = os.path.join(SRC_PATH, SUB_FOLDER, file_type_name, file_name + '.cpp')

    files = [header_file, source_file]

    for file in files:
        if os.path.exists(file):
            print('File already exists: ' + file)
            sys.exit(1)

    for i, file in enumerate(files):
        with open(file, 'w') as f:
            f.write(FILE_TEMPLATES.get(file_type, DEFAULT_FILE_TEMPLATE)[i].format(NAMESPACE_NAME, file_name, file_type_name))

    # Find place that begins with #begin file_type and ends with #end, and append the source file to it
    place_line_idx = -1
    place_line_find_str = "#place " + file_type
    with open(CMAKE_FILE_PATH, 'r') as f:
        lines = f.readlines()
        for i, line in enumerate(lines):
            if line.strip() == place_line_find_str:
                place_line_idx = i
                break

    if place_line_idx == -1 or lines is None:
        print('Could not find place to insert the new file')
        sys.exit(1)

    # make copy just in case
    with open(CMAKE_FILE_PATH + '.bak', 'w') as f:
        f.writelines(lines)

    with open(CMAKE_FILE_PATH, 'w') as f:
        f.writelines(lines[:place_line_idx] + ['    ' + source_file.replace("\\", "/") + "\n"] + lines[place_line_idx:])





 

