#!/usr/bin/env python3

import os
import subprocess

SHADER_DIRECTORIES = ['src/rendering/shaders', 'src/rendering/effects/shaders']

def find_files(dirs: list[str]) -> list[str]:
    files: list[str] = []

    for dir in dirs:
        for (dirpath, _, filenames) in os.walk(dir):
            for filename in filenames:
                files.append(dirpath + "/" + filename)

    return files

def compile_shader(path: str) -> list[str]:
    out = f'{path}.u32'

    # -V: create SPIR-V binary
    # -x: save binary output as text-based 32-bit hexadecimal numbers
    # -o: output file
    result = subprocess.run(["glslc", "-c", "-mfmt=num", path, "-o", out])

    if result.returncode != 0:
        return []

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
    path: str
    vertex_code: list[str]
    fragment_code: list[str]
    compute_code: list[str]
    is_compute: bool = False

def generate_header(shader: ShaderData):
    define: str = shader.name.upper() + '_SHADER_GEN_H'
    name: str = shader.name.title() + 'Shader'
    name = name.replace("_", "")

    body: str = ""

    if shader.is_compute == True:
        compute: str = ','.join(shader.compute_code)
        body += f"\tconstexpr static const uint32_t computeCode[] = {{{compute}}};"
    else:
        vertex: str = ','.join(shader.vertex_code)
        body += f"\tconstexpr static const uint32_t vertexCode[] = {{{vertex}}};"

        fragment: str = ','.join(shader.fragment_code)
        body += f"\n\tconstexpr static const uint32_t fragmentCode[] = {{{fragment}}};"

    header = f"""// THIS FILE IS GENERATED; DO NOT EDIT!
#ifndef {define}
#define {define}

#include <cstdint>

class {name} {{
public:
{body}
}};

#endif // !{define}
"""


    path, _ = os.path.splitext(shader.path)

    file = open(f'{path}.gen.h', 'w')
    file.write(header)
    file.close()

def main():
    names: list[str] = []
    data: list[ShaderData] = []

    for file in find_files(SHADER_DIRECTORIES):
        filename = file.split('/')[-1]
        name, ext = os.path.splitext(filename)

        if ext != ".vert" and ext != ".frag" and ext != ".comp":
            continue

        if name not in names:
            names.append(name)

            shader = ShaderData()
            shader.name = name
            shader.path = file
            data.append(shader)

        idx: int = names.index(name)

        spirv = compile_shader(f'{file}')

        if ext == ".vert":
            data[idx].vertex_code = spirv
        elif ext == ".frag":
            data[idx].fragment_code = spirv
        else:
            data[idx].is_compute = True;
            data[idx].compute_code = spirv


    for shader in data:
        generate_header(shader)

main()
