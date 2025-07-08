import tkinter as tk
import math
from PIL import ImageFont, ImageDraw, Image, ImageTk

FONT_FILE = 'palm-os-bold.ttf'
SCALING = 4
WIDTH = 512
INITIAL_SIZE = 20

#ascent, descent = imageFont.getmetrics()
#print(ascent, descent)
# draw.rectangle([0, offset, width, offset+ascent-1], col, col)
# draw.rectangle([0, offset+ascent, width, offset+ascent+descent-1], col, col)

class FontConfig:
    def __init__(self):
        self.file = FONT_FILE
        self.size = INITIAL_SIZE
        self.height = INITIAL_SIZE
        self.offset = 0


font = FontConfig()


def get_char_data(font, char):
    imgfont = ImageFont.truetype(font.file, font.size)
    # First get the char width
    dummy_img = Image.new(mode='1', size=(128,128), color=0)
    draw = ImageDraw.Draw(dummy_img)
    char_width = draw.textlength(char, imgfont)
    if char_width != int(char_width):
        print("Warning: char_width is not an integer")
    char_width = int(char_width) - 1 # remove space
    # Then create an image of the correct size
    image = Image.new(mode='1', size=(char_width,font.height), color=0)
    draw = ImageDraw.Draw(image)
    draw.text((0, font.offset), char, font=imgfont, fill=255)

    img = ImageTk.PhotoImage(image)
    charview.configure(image=img)
    charview.image=img

    pixels = list(image.getdata())
    data = []

    bytes_per_col = math.ceil(font.height / 8)
    for col_byte in range(bytes_per_col):
        for x in range(char_width):
            byteout = 0
            for by in range(8):
                y = 8*col_byte + by
                if y >= font.height:
                    pixel = 0
                else:
                    idx = y*char_width + x
                    pixel = 1 if pixels[idx] else 0
                byteout >>= 1
                byteout |= (pixel << 7)
            print(f"{byteout:02x} ", end='')
            data.append(byteout)
    print()



    
def draw_text(text, fontcfg):
    font = ImageFont.truetype(fontcfg.file, fontcfg.size)

    image = Image.new(mode='1', size=(WIDTH,fontcfg.height), color=0)
    draw = ImageDraw.Draw(image)
    draw.text((0, fontcfg.offset), text, font=font, fill=255)
    print(draw.textlength(text, font))

    image = image.resize((SCALING*WIDTH, SCALING*fontcfg.height), Image.Resampling.NEAREST)
    img = ImageTk.PhotoImage(image)
    return img


def update():
    font.size = int(size_spin.get())
    font.offset = int(offset_spin.get())
    font.height = int(height_spin.get())
    
    image = draw_text("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ", font)
    line1.configure(image=image)
    line1.image=image

    image2 = draw_text("The quick brown fox jumps over the lazy dog. 0123456789", font)
    line2.configure(image=image2)
    line2.image=image2

    get_char_data(font, 'a')


window = tk.Tk()
#window.geometry("500x500") # (optional)    


sidebar = tk.Frame(window, width=200, height=400)
sidebar.pack(side=tk.LEFT, fill=tk.Y)


size = tk.StringVar(window)
size.set(str(INITIAL_SIZE))
size_label = tk.Label(sidebar, text='Font size')
size_label.pack()
size_spin = tk.Spinbox(sidebar, state='readonly', command=update, from_=4, to=64, textvariable=size)
size_spin.pack()

offset = tk.StringVar(window)
offset.set("0")
offset_label = tk.Label(sidebar, text='Y offset')
offset_label.pack()
offset_spin = tk.Spinbox(sidebar, state='readonly', command=update, from_=-16, to=16, textvariable=offset)
offset_spin.pack()

height = tk.StringVar(window)
height.set(str(INITIAL_SIZE))
height_label = tk.Label(sidebar, text='Bitmap height')
height_label.pack()
height_spin = tk.Spinbox(sidebar, state='readonly', command=update, from_=0, to=32, textvariable=height)
height_spin.pack()

line1 = tk.Label(window, image = None)
line1.pack()

line2 = tk.Label(window, image = None)
line2.pack()

charview = tk.Label(window, image = None)
charview.pack()

update()

window.mainloop()