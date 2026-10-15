import glob, os
root = r'C:\Program Files (x86)\Starbreeze Studios\Riddick EFBB'
files = [f for f in glob.glob(root + r'\**\*', recursive=True)
         if f.lower().endswith(('.dll', '.exe'))]
pats = [b'VERTEXFRAMES', b'BONEDEFORM', b'TRIANGLES', b'NODEINDICES',
        b'SKELETON', b'TVERTEXFRAMES', b'PRIMITIVES', b'CLUSTERS',
        b'.xmd', b'MOS MODEL', b'Model', b'viewmodel', b'ViewModel',
        b'EXTENDEDMODEL', b'SURFACES', b'NODES', b'EDGETRIS']
for p in pats:
    hits = []
    for f in files:
        try:
            d = open(f, 'rb').read()
        except OSError:
            continue
        i = d.find(p)
        if i >= 0:
            hits.append((os.path.relpath(f, root), hex(i)))
    print(p.decode(), '->', hits[:8])
