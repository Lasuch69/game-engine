#!/usr/bin/env python3

import os
import subprocess

shader_dir = 'src/rendering/shaders/'

def find_files(dir: str) -> list[str]:
    files: list[str] = []
    for (_, _, filenames) in os.walk(dir):
        files.extend(filenames)
        break

    return files

def compile_shader(path: str) -> list[str]:
    out = f'{path}.u32'

    # -V: create SPIR-V binary
    # -x: save binary output as text-based 32-bit hexadecimal numbers
    # -o: output file
    subprocess.run(["glslc", "-c", "-mfmt=num", path, "-o", out])
    file = open(out, 'r')

    txt: str = ''
    line = file.readline()
    while line != '':
        txt += line.replace('\n', '').replace('\t', '')
        line = file.readline()

    file.close()
    os.remove(out)

    return txt.split(',')

class ShaderData:
    name: str
    vertex_code: list[str]
    fragment_code: list[str]

def generate_header(shader: ShaderData):
    define: str = shader.name.upper() + '_SHADER_GEN_H'
    name: str = shader.name.title() + 'Shader'

    header = f"""// THIS FILE IS GENERATED; DO NOT EDIT!
#ifndef {define}
#define {define}

#include <cstdint>

class {name} {{
public:
    constexpr static const uint32_t vertexCode[] = {{{','.join(shader.vertex_code)}}};
    constexpr static const uint32_t fragmentCode[] = {{{','.join(shader.fragment_code)}}};
}};

#endif // !{define}
"""

    file = open(f'{shader_dir}{shader.name}.gen.h', 'w')
    file.write(header)
    file.close()

def main():
    names: list[str] = []
    data: list[ShaderData] = []

    for filename in find_files(shader_dir):
        name, ext = os.path.splitext(filename)

        if ext != ".vert" and ext != ".frag":
            continue

        if name not in names:
            names.append(name)

            shader = ShaderData()
            shader.name = name
            data.append(shader)

        idx: int = names.index(name)

        spirv = compile_shader(f'{shader_dir}{filename}')

        if ext == ".vert":
            data[idx].vertex_code = spirv
        else:
            data[idx].fragment_code = spirv


    for shader in data:
        generate_header(shader)

main()
