import os
from PIL import Image

def png_to_girouette_ascii(folder_path):
  with open("./bus/display/glyphs_ext.h", "w") as f:
    f.write("#include <stdint.h>\n\n")
    f.write("typedef struct { const uint32_t *cols; uint8_t width; } Glyph;\n\n")
    
    found_ascii = {}

    for filename in sorted(os.listdir(folder_path)):
      if filename.endswith(".png"):
        name = os.path.splitext(filename)[0]
        
        if len(name) == 1:
          ascii_code = ord(name)
        else:
          try:
            ascii_code = int(name) # Si le fichier s'appelle '33.png'
          except ValueError:
            continue

        img = Image.open(os.path.join(folder_path, filename)).convert('RGBA')
        width, height = img.size
        
        # Génération du tableau de colonnes
        var_name = f"bmp_ascii_{ascii_code}"
        found_ascii[ascii_code] = (var_name, width)
        
        f.write(f"const uint32_t {var_name}[] = {{\n  ")
        for x in range(width):
          col_value = 0
          for y in range(height):
            _, _, _, a = img.getpixel((x, y))
            if a > 128:
              col_value |= (1 << (y + 4)) # Centrage vertical
          f.write(f"0x{col_value:06x}, ")
        f.write("\n};\n\n")

    # Création du grand tableau de 256 entrées
    f.write("const Glyph bus_font[256] = {\n")
    for code in range(256):
      if code in found_ascii:
        var_name, width = found_ascii[code]
        f.write(f"  [{code}] = {{ {var_name}, {width} }},\n")
      else:
        # Caractère vide par défaut (espace)
        f.write(f"  [{code}] = { { 0, 0 } },\n")
    f.write("};\n")

png_to_girouette_ascii("./bus/display/glyphs/ext")