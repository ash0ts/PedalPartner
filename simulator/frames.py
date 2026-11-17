"""Standard-library PPM/PNG helpers shared by the simulator server, tests and tools."""
import struct,zlib

WIDTH=HEIGHT=240

def ppm_rgb(data):
    """Return the raw RGB bytes of a binary P6 240x240 PPM."""
    header,body=data.split(b'\n',3)[:3],data.split(b'\n',3)[3]
    if header!=[b'P6',b'240 240',b'255'] or len(body)!=WIDTH*HEIGHT*3:
        raise ValueError('Unexpected PPM frame')
    return body

def rgb_png(rgb):
    def chunk(kind,body):
        return struct.pack('>I',len(body))+kind+body+struct.pack('>I',zlib.crc32(kind+body)&0xffffffff)
    raw=b''.join(b'\0'+rgb[y*WIDTH*3:(y+1)*WIDTH*3] for y in range(HEIGHT))
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',WIDTH,HEIGHT,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(raw,9))+chunk(b'IEND',b'')

def ppm_png(data):
    return rgb_png(ppm_rgb(data))

def png_rgb(data):
    """Decode a PNG written by rgb_png (8-bit RGB, filter 0 rows) back to RGB bytes."""
    if not data.startswith(b'\x89PNG\r\n\x1a\n'):
        raise ValueError('Not a PNG')
    pos=8;idat=b''
    while pos<len(data):
        length,kind=struct.unpack('>I4s',data[pos:pos+8]);body=data[pos+8:pos+8+length];pos+=12+length
        if kind==b'IHDR':
            w,h,depth,color=struct.unpack('>IIBB',body[:10])
            if (w,h,depth,color)!=(WIDTH,HEIGHT,8,2):
                raise ValueError('Unsupported PNG layout')
        elif kind==b'IDAT':
            idat+=body
    raw=zlib.decompress(idat);rows=[]
    for y in range(HEIGHT):
        row=raw[y*(WIDTH*3+1):(y+1)*(WIDTH*3+1)]
        if row[0]!=0:
            raise ValueError('Only filter 0 rows are supported')
        rows.append(row[1:])
    return b''.join(rows)

def diff_count(a,b):
    """Number of differing pixels between two RGB buffers."""
    return sum(1 for i in range(0,len(a),3) if a[i:i+3]!=b[i:i+3])
