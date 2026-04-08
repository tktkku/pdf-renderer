#!/usr/bin/env python3

def convert_glyphlist_to_header():
    input_file = "../agl-aglfn/glyphlist.txt"
    output_file = "../render/src/pdf-glyph-list.h"
    
    glyphs = []
    
    with open(input_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            
            if not line or line.startswith('#'):
                continue
            
            parts = line.split(';')
            if len(parts) != 2:
                continue
            
            glyph_name = parts[0].strip()
            unicode_hex = parts[1].strip()
            
            try:
                unicode_value = int(unicode_hex, 16)
                glyphs.append((glyph_name, unicode_value))
            except ValueError:
                continue
    
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write("#ifndef GLYPHLIST_H\n")
        f.write("#define GLYPHLIST_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("typedef struct {\n")
        f.write("    const char* name;\n")
        f.write("    uint32_t unicode;\n")
        f.write("} GlyphEntry;\n\n")
        f.write("static const GlyphEntry glyphlist[] = {\n")
        
        for glyph_name, unicode_value in glyphs:
            f.write(f'    {{ "{glyph_name}", 0x{unicode_value:04X} }},\n')
        
        f.write("};\n\n")
        f.write(f"#define GLYPHLIST_SIZE {len(glyphs)}\n\n")
        f.write("#endif // GLYPHLIST_H\n")
    
    print(f"Successfully converted {len(glyphs)} glyphs from {input_file} to {output_file}")

if __name__ == "__main__":
    convert_glyphlist_to_header()
