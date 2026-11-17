"""Standard-library PNG reader/writer for the art pipeline (no Pillow required).

decode(data) -> (width, height, pixels) where pixels is a list of (r, g, b, a) tuples, row-major.
Supports 8-bit greyscale, grey+alpha, RGB, RGBA and palette images with optional tRNS, all five
scanline filters, non-interlaced only (what every pixel editor exports by default).
encode_rgba(width, height, pixels) -> bytes.
"""
import struct,zlib

def _chunks(data):
    if not data.startswith(b'\x89PNG\r\n\x1a\n'):raise ValueError('not a PNG')
    pos=8
    while pos<len(data):
        length,kind=struct.unpack('>I4s',data[pos:pos+8]);yield kind,data[pos+8:pos+8+length];pos+=12+length

def _paeth(a,b,c):
    p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c)
    return a if pa<=pb and pa<=pc else b if pb<=pc else c

def decode(data):
    width=height=depth=ctype=interlace=None;palette=[];trns=b'';idat=[]
    for kind,body in _chunks(data):
        if kind==b'IHDR':width,height,depth,ctype,_,_,interlace=struct.unpack('>IIBBBBB',body)
        elif kind==b'PLTE':palette=[tuple(body[i:i+3]) for i in range(0,len(body),3)]
        elif kind==b'tRNS':trns=body
        elif kind==b'IDAT':idat.append(body)
    if depth!=8:raise ValueError(f'only 8-bit PNGs are supported (got {depth}-bit)')
    if interlace:raise ValueError('interlaced PNGs are not supported; export without interlacing')
    bpp={0:1,2:3,3:1,4:2,6:4}[ctype];stride=width*bpp
    raw=zlib.decompress(b''.join(idat));rows=[];prev=bytearray(stride);pos=0
    for _ in range(height):
        f=raw[pos];line=bytearray(raw[pos+1:pos+1+stride]);pos+=1+stride
        for i in range(stride):
            a=line[i-bpp] if i>=bpp else 0;b=prev[i];c=prev[i-bpp] if i>=bpp else 0
            if f==1:line[i]=(line[i]+a)&255
            elif f==2:line[i]=(line[i]+b)&255
            elif f==3:line[i]=(line[i]+((a+b)>>1))&255
            elif f==4:line[i]=(line[i]+_paeth(a,b,c))&255
        rows.append(bytes(line));prev=line
    px=[]
    for line in rows:
        for i in range(0,stride,bpp):
            if ctype==6:px.append(tuple(line[i:i+4]))
            elif ctype==2:px.append((line[i],line[i+1],line[i+2],255))
            elif ctype==0:px.append((line[i],line[i],line[i],255))
            elif ctype==4:px.append((line[i],line[i],line[i],line[i+1]))
            else:
                k=line[i];r,g,b=palette[k];px.append((r,g,b,trns[k] if k<len(trns) else 255))
    return width,height,px

def _chunk(kind,body):
    return struct.pack('>I',len(body))+kind+body+struct.pack('>I',zlib.crc32(kind+body)&0xffffffff)

def encode_rgba(width,height,pixels):
    raw=b''.join(b'\0'+bytes(c for p in pixels[y*width:(y+1)*width] for c in p) for y in range(height))
    return b'\x89PNG\r\n\x1a\n'+_chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,6,0,0,0))+_chunk(b'IDAT',zlib.compress(raw,9))+_chunk(b'IEND',b'')
