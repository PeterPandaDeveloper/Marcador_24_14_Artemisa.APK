import sys
from PIL import Image

def add_padding(image_path, scale_factor=0.6):
    img = Image.open(image_path).convert("RGBA")
    w, h = img.size
    
    new_w = int(w * scale_factor)
    new_h = int(h * scale_factor)
    
    img_resized = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
    
    new_img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    
    offset_x = (w - new_w) // 2
    offset_y = (h - new_h) // 2
    
    new_img.paste(img_resized, (offset_x, offset_y), img_resized)
    new_img.save(image_path)
    print("Padding added successfully.")

if __name__ == "__main__":
    add_padding(sys.argv[1])
