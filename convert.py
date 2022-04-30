from base64 import b64encode
from io import BytesIO
from math import ceil
from sys import argv
from PIL import Image

with open(argv[1], 'rb') as f:
    wasm = f.read()

def pngstring(a):
    w = 1024
    img = Image.new('P', (w, ceil(len(a) / w)))
    img.putpalette(sum([[i, 0, 0] for i in range(256)], []))
    img.putdata(a)
    buf = BytesIO()
    img.save(buf, 'PNG')
    return b64encode(buf.getvalue())

with open(argv[2], 'wb') as f:
    f.write(b'export const imageSource = \'data:image/png;base64,' + pngstring(wasm) + b'\';\n')
    f.write(b'export const imageSource_size = ' + bytes(str(len(wasm)), 'ascii') + b';\n')
