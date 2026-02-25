import os
from PIL import Image

def png_to_c_array(folder_path):
  with open("./bus/display/glyphs_ext.c", "w") as f:
    f.write("#include <stdint.h>\n\ntypedef struct {\n    const uint8_t *data; // Pointeur vers le tableau de pixels\n    uint8_t width;       // Largeur réelle du PNG\n    uint8_t height;      // Hauteur (ici 24)\n} Glyph;\n\n")

    structure = "Glyph bus_font[256] = {"
    
    for filename in sorted(os.listdir(folder_path)):
      if filename.endswith(".png"):
        img = Image.open(os.path.join(folder_path, filename)).convert('1') # 1-bit (Noir et Blanc)
        width, height = img.size
        name = os.path.splitext(filename)[0]
        
        f.write(f"// Image: {filename} ({width}x{height})\n")
        f.write(f"const uint8_t bitmap_{name}[] = {{\n    ")
        
        # Conversion en octets (8 pixels par octet)
        pixels = list(img.getdata())
        for i in range(0, len(pixels), 8):
          byte = 0
          for bit in range(8):
            if i + bit < len(pixels):
              # Si le pixel est blanc (allumé pour une LED), on met le bit à 1
              if pixels[i + bit] > 0:
                byte |= (1 << (7 - bit))
          f.write(f"0x{byte:02x}, ")
          if (i // 8 + 1) % 12 == 0: f.write("\n    ")

          structure += f"\n  ['{name}'] = {{ bitmap_{name}, {width}, {height} }}"
        
        f.write("\n};\n\n")
    
    structure += "\n};\n\n"

    f.write(structure)

png_to_c_array("./bus/display/glyphs/ext")