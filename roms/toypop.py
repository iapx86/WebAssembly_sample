#
#   Toypop
#

from base64 import b64encode
from io import BytesIO
from math import ceil
from sys import argv
from zipfile import ZipFile
from PIL import Image

with ZipFile(argv[1]) as z:
    merge = lambda even, odd : bytes([odd[i // 2] if i % 2 else even[i // 2] for i in range(len(even) * 2)])
    prg1 = z.read('tp1-2.5b') + z.read('tp1-1.5c')
    prg2 = z.read('tp1-3.2c')
    prg3 = merge(z.read('tp1-4.8c'), z.read('tp1-5.10c'))
    bg = z.read('tp1-7.5p')
    obj = z.read('tp1-6.9t')
    red = z.read('tp1-3.1r')
    green = z.read('tp1-2.1s')
    blue = z.read('tp1-1.1t')
    bgcolor = z.read('tp1-4.5l')
    objcolor = z.read('tp1-5.2p')
    snd = z.read('tp1-6.3d')

rom = prg1 + prg2 + prg3 + bg + obj + red + green + blue + bgcolor + objcolor + snd

def pngstring(a):
    w = 1024
    img = Image.new('P', (w, ceil(len(a) / w)))
    img.putpalette(sum([[i, 0, 0] for i in range(256)], []))
    img.putdata(a)
    buf = BytesIO()
    img.save(buf, 'PNG')
    return b64encode(buf.getvalue())

with open(argv[2], 'wb') as f:
    f.write(b'export const ROM = \'data:image/png;base64,' + pngstring(rom) + b'\';\n')
