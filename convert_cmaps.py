#!/usr/bin/python
# -*- coding: UTF-8 -*-

import sys
import re
import os
import errno

def process_cid_file(input_file):
    # read file
    with open(input_file, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()
    
    # save all blocks
    blocks = {
        'codespacerange': {'data': [], 'num_lines': 0},
        'notdefrange': {'data': [], 'num_lines': 0},
        'cidchar': {'data': [], 'num_lines': 0},
        'cidrange': {'data': [], 'num_lines': 0}
    }
    
    i = 0
    while i < len(lines):
        line = lines[i].strip()
        
        # match block start
        block_match = re.match(r'^(\d+)\s+begin(codespacerange|notdefrange|cidchar|cidrange)', line, re.IGNORECASE)
        if block_match:
            num_lines = int(block_match.group(1))
            block_type = block_match.group(2).lower()
            start_index = i + 1
            end_index = start_index + num_lines
            
            # check if enough lines
            if end_index >= len(lines):
                i += 1
                continue
                
            # check end marker
            end_marker = lines[end_index].strip().lower()
            if f"end{block_type}" not in end_marker:
                i += 1
                continue
                
            # save block data
            block_data = lines[start_index:end_index]
            blocks[block_type]['data'].extend(block_data)
            blocks[block_type]['num_lines'] += num_lines
            i = end_index
        else:
            i += 1
    
    # process each block type
    base_name = os.path.basename(input_file)
    file_name_no_ext = os.path.splitext(base_name)[0]
    file_prefix = re.sub(r'\W', '_', file_name_no_ext).upper()
    output = "#include \"../pdf-private.h\"\n\n"
    
    # Process codespacerange
    codespace_ptr = "NULL"
    codespace_len = 0
    if blocks['codespacerange']['num_lines'] > 0:
        entries = []
        for line in blocks['codespacerange']['data']:
            line = line.strip()
            if not line:
                continue
            match = re.match(r'<([0-9a-fA-F]+)>\s+<([0-9a-fA-F]+)>', line)
            if match:
                hex1 = match.group(1).lower()
                hex2 = match.group(2).lower()
                byte_len = len(hex1) // 2
                src_start = f'0x{hex1}'
                src_end = f'0x{hex2}'
                entries.append("{" + f"{byte_len}, {src_start}, {src_end}" + "}")
        
        array_name = f"code_range_map_{file_prefix}"
        output += f"static pdf_code_range_map_t {array_name}[] = {{\n"
        for entry in entries:
            output += f"    {entry},\n"
        output += "};\n\n"
        codespace_ptr = array_name
        codespace_len = len(entries)
    
    # Process notdefrange
    notdef_ptr = "NULL"
    notdef_len = 0
    if blocks['notdefrange']['num_lines'] > 0:
        entries = []
        for line in blocks['notdefrange']['data']:
            line = line.strip()
            if not line:
                continue
            match = re.match(r'<([0-9a-fA-F]+)>\s+<([0-9a-fA-F]+)>\s+(\d+)', line)
            if match:
                hex1 = f'0x{match.group(1).lower()}'
                hex2 = f'0x{match.group(2).lower()}'
                dst_start = f'{hex(int(match.group(3)))}'
                entries.append("{" + f"{hex1}, {hex2}, {dst_start}" + "}")
        
        array_name = f"not_def_range_{file_prefix}"
        output += f"static pdf_char_range_map_t {array_name}[] = {{\n"
        for entry in entries:
            output += f"    {entry},\n"
        output += "};\n\n"
        notdef_ptr = array_name
        notdef_len = len(entries)
    
    # Process cidchar
    unicode_ptr = "NULL"
    unicode_len = 0
    if blocks['cidchar']['num_lines'] > 0:
        entries = []
        for line in blocks['cidchar']['data']:
            line = line.strip()
            if not line:
                continue
            match = re.match(r'<([0-9a-fA-F]+)>\s+(\d+)', line)
            if match:
                unicode_val = f'0x{match.group(1).lower()}'
                cid_val = f'{hex(int(match.group(2)))}'
                entries.append("{" + f"{cid_val}, {unicode_val}" + "}")
        
        array_name = f"unicode_map_{file_prefix}"
        output += f"static pdf_unicode_map_t {array_name}[] = {{\n"
        for entry in entries:
            output += f"    {entry},\n"
        output += "};\n\n"
        unicode_ptr = array_name
        unicode_len = len(entries)
    
    # Process cidrange
    char_range_ptr = "NULL"
    char_range_len = 0
    if blocks['cidrange']['num_lines'] > 0:
        entries = []
        for line in blocks['cidrange']['data']:
            line = line.strip()
            if not line:
                continue
            match = re.match(r'<([0-9a-fA-F]+)>\s+<([0-9a-fA-F]+)>\s+(\d+)', line)
            if match:
                hex1 = f'0x{match.group(1).lower()}'
                hex2 = f'0x{match.group(2).lower()}'
                dst_start = f'{hex(int(match.group(3)))}'
                entries.append("{" + f"{hex1}, {hex2}, {dst_start}" + "}")
        
        array_name = f"char_range_map_{file_prefix}"
        output += f"static pdf_char_range_map_t {array_name}[] = {{\n"
        for entry in entries:
            output += f"    {entry},\n"
        output += "};\n\n"
        char_range_ptr = array_name
        char_range_len = len(entries)
    
    # Generate cmap struct
    cmap_name = file_name_no_ext
    worldwide = "true" #if "Identity" in cmap_name else "false"
    cmap_var_name = f"cmap_{file_prefix}"
    
    output += f"pdf_cmap_t {cmap_var_name} = {{\n"
    output += f"    \"{cmap_name}\",    //name\n"
    output += f"    {worldwide},        //worldwide\n"
    output += f"    {unicode_len},      //unicode_map_len\n"
    output += f"    {unicode_ptr},      //unicode_map\n"
    output += f"    {char_range_len},   //char_range_map_len\n"
    output += f"    {char_range_ptr},   //char_range_map\n"
    output += f"    {notdef_len},       //not_def_range_len\n"
    output += f"    {notdef_ptr},       //not_def_range\n"
    output += f"    {codespace_len},    //code_range_map_len\n"
    output += f"    {codespace_ptr},    //code_range_map\n"
    output += "     NULL//next\n};\n\n"
    
    total_lines = sum(block['num_lines'] for block in blocks.values())
    print(f"processed {total_lines} lines")
    return output, cmap_name, cmap_var_name

if __name__ == "__main__":
    cmap_paths = [
        'cmap-resources/Adobe-CNS1-7/CMap',
        'cmap-resources/Adobe-GB1-6/CMap',
        'cmap-resources/Adobe-Identity-0/CMap',
        'cmap-resources/Adobe-Japan1-7/CMap',
        'cmap-resources/Adobe-Korea1-2/CMap',
        'cmap-resources/Adobe-KR-9/CMap',
        'cmap-resources/Adobe-Manga1-0/CMap',
        'mapping-resources-pdf/pdf2unicode'
    ]
    output_path = './parser/CMap'
    if not os.path.exists(output_path):
        try:
            os.makedirs(output_path, exist_ok=True)
        except OSError as e:
            if e.errno != errno.EEXIST:
                raise
    
    cmap_index = []  # Store (filename, cmap_var_name) pairs
    
    for folder_path in cmap_paths:
        all_items = os.listdir(folder_path)
        file_names = [os.path.join(folder_path, f) 
                      for f in all_items 
                      if os.path.isfile(os.path.join(folder_path, f))]
        for file_name in file_names:
            base_file_name = os.path.basename(file_name)
            output_filename = os.path.join(output_path, base_file_name + '.h')
            try:
                print(f"processing file {file_name}")
                result, cmap_name, cmap_var = process_cid_file(file_name)
                with open(output_filename, 'w', encoding='utf-8') as f:
                    f.write(result)
                cmap_index.append((cmap_name, cmap_var))
                print(f"generated file: {output_filename}")
            except Exception as e:
                print(f"process file error: {str(e)}")
                sys.exit(1)
    
    # 按名称排序索引
    cmap_index_sorted = sorted(cmap_index, key=lambda x: x[0])
    
    # Generate CMaps.h
    cmaps_h = os.path.join(output_path, "CMaps.h")
    with open(cmaps_h, 'w', encoding='utf-8') as f:
        f.write("#ifndef CMAPS_H\n#define CMAPS_H\n\n")
        f.write("#include \"../pdf-private.h\"\n\n")
        
        # Include all generated headers
        for folder_path in cmap_paths:
            all_items = os.listdir(folder_path)
            for item in all_items:
                if os.path.isfile(os.path.join(folder_path, item)):
                    f.write(f'#include "{item}.h"\n')
        
        # Create global index table (sorted by name)
        f.write("\n// Global CMAP index (sorted by name)\n")
        f.write("static const struct {\n")
        f.write("    const char *name;\n")
        f.write("    pdf_cmap_t *cmap;\n")
        f.write(f'}} g_CMAP_INDEX[{len(cmap_index_sorted)}] = {{\n')
        
        # 使用排序后的索引
        for name, var in cmap_index_sorted:
            f.write(f'    {{ "{name}", &{var} }},\n')
        
        f.write("};\n\n")
        f.write("#endif // CMAPS_H\n")
    
    print(f"generated CMaps.h: {cmaps_h}")