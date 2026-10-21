"""Generate local background repairs from the player's game data and downloaded mods.
No original game/third-party asset bytes are embedded in this program.
"""
from pathlib import Path
import argparse, json, math, struct, sys, zipfile
import xml.etree.ElementTree as E
sys.path.insert(0,str(Path(__file__).resolve().parent))
sys.dont_write_bytecode=True
import mpyq
p=argparse.ArgumentParser()
p.add_argument('--mods',type=Path,required=True)
p.add_argument('--game-archive',type=Path,required=True)
p.add_argument('--output',type=Path,required=True)
args=p.parse_args()
ASSETS=Path(args.output)/'geometry-work'
ASSETS.mkdir(parents=True,exist_ok=True)
# Preserve the original pack's precedence: Full 3D overrides Background 3DS.
for filename in ("Djipi's 3DE - 26 Background 3DS.otr","Djipi's 3DE - 32 Background Full 3D (OPTIONAL).otr"):
    archive=mpyq.MPQArchive(str(Path(args.mods)/filename))
    for raw in archive.files:
        name=raw.decode('utf-8').replace('\\','/')
        if not any(k in name for k in ('market','shrine','hairal_niwa')):continue
        if not name.startswith('alt/') or any(x in ('..','') for x in name.split('/')):raise ValueError('Unexpected resource path')
        entry=archive.get_hash_table_entry(name)
        if entry is None:raise ValueError('Missing MPQ resource')
        block=archive.block_table[entry.block_table_index]
        if not block.flags & (mpyq.MPQ_FILE_COMPRESS | mpyq.MPQ_FILE_ENCRYPTED | mpyq.MPQ_FILE_IMPLODE):
            if block.archived_size!=block.size:raise ValueError('Invalid MPQ block size')
            archive.file.seek(archive.header['offset']+block.offset)
            data=archive.file.read(block.size)
        else:data=archive.read_file(name)
        if not data or len(data)>8000000:raise ValueError('Unsupported geometry resource')
        target=ASSETS/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
    archive.file.close()
MARKET=ASSETS/'alt/scenes/shared/market_day_scene'
resources={};audit=[]
materials={"wall":"MarketPlace32","stone":"MarketPlace19","roof":"MarketPlace10",
           "window":"MarketPlace06","ground":"TempleTime11","timber":"MarketPlace30"}
sizes={}
for role,suffix in materials.items():
    p=MARKET/("mat_market_day_room_0DL_0057D8_"+suffix)
    tree=E.fromstring(p.read_bytes())
    tile=tree.find("SetTileSize")
    sizes[role]=((int(tile.attrib["Lrs"])+4)//4,(int(tile.attrib["Lrt"])+4)//4)
    resources[f"lighting/infill/material-{role}"]=E.tostring(tree)

class Mesh:
    def __init__(self,name):
        self.name=name;self.groups={};self.tris=0;self.bounds=[]
    def face(self,role,points,repeat=(1,1),color=210,flip=True):
        # Outward winding is explicit. Reverse surfaces are real faces on the
        # opposite side of each building, not a blanket culling disable.
        w,h=sizes[role]
        if flip:points=list(reversed(points))
        repeat=(min(repeat[0],32700/(w*32)),min(repeat[1],32700/(h*32)))
        uv=[(0,repeat[1]),(repeat[0],repeat[1]),(repeat[0],0),(0,0)]
        verts=[dict(zip(("X","Y","Z"),[round(v) for v in p]))|
               dict(S=round(uv[i][0]*w*32),T=round(uv[i][1]*h*32),R=color,G=color,B=color,A=255)
               for i,p in enumerate(points)]
        group=self.groups.setdefault(role,[])
        group.append(verts);self.tris+=len(points)-2;self.bounds+=points
    def box(self,x0,x1,y0,y1,z0,z1,role="wall",tile=180):
        u=(x1-x0)/tile;v=(y1-y0)/tile;w=(z1-z0)/tile
        self.face(role,[(x0,y0,z0),(x1,y0,z0),(x1,y1,z0),(x0,y1,z0)],(u,v))
        self.face(role,[(x1,y0,z1),(x0,y0,z1),(x0,y1,z1),(x1,y1,z1)],(u,v))
        self.face(role,[(x0,y0,z1),(x0,y0,z0),(x0,y1,z0),(x0,y1,z1)],(w,v))
        self.face(role,[(x1,y0,z0),(x1,y0,z1),(x1,y1,z1),(x1,y1,z0)],(w,v))
        self.face(role,[(x0,y1,z0),(x1,y1,z0),(x1,y1,z1),(x0,y1,z1)],(u,w))
    def building(self,x0,x1,z0,z1,height,rise):
        self.box(x0,x1,-110,60,z0,z1,"stone",200)
        self.box(x0,x1,60,height,z0,z1,"wall",240)
        # Pitched roof, ridge along Z. Eaves and gables close every side.
        mid=(x0+x1)/2;a=x0-20;b=x1+20;z0-=20;z1+=20
        self.face("roof",[(a,height,z0),(mid,height+rise,z0),(mid,height+rise,z1),(a,height,z1)],((b-a)/220,(z1-z0)/180))
        self.face("roof",[(mid,height+rise,z0),(b,height,z0),(b,height,z1),(mid,height+rise,z1)],((b-a)/220,(z1-z0)/180))
        self.face("wall",[(a,height,z0),(b,height,z0),(mid,height+rise,z0)])
        self.face("wall",[(b,height,z1),(a,height,z1),(mid,height+rise,z1)])
        # Small paired windows and timber strips make the outer facades legible
        # at close free-camera angles, using the existing town material atlas.
        for z in range(round(z0+100),round(z1-70),190):
            for x,reverse in ((x0-1,False),(x1+1,True)):
                points=[(x,130,z-32),(x,130,z+32),(x,220,z+32),(x,220,z-32)]
                if reverse:points.reverse()
                self.face("window",points,(1,1),235,flip=False)
        self.box(x0-2,x1+2,67,79,z0+18,z1-18,"timber",160)
    def export(self):
        root=E.Element("DisplayList",Version="0")
        E.SubElement(root,"ClearGeometryMode",G_LIGHTING="1")
        for role,faces in self.groups.items():
            E.SubElement(root,"CallDisplayList",Path=f"lighting/infill/material-{role}")
            vertex=E.Element("Vertex",Version="0");dl=E.Element("DisplayList",Version="0")
            offset=0
            for points in faces:
                E.SubElement(dl,"LoadVertices",Path=f"lighting/infill/{self.name}-{role}-vtx",
                             VertexBufferIndex="0",VertexOffset=str(offset),Count=str(len(points)))
                for point in points:E.SubElement(vertex,"Vtx",**{k:str(v) for k,v in point.items()})
                E.SubElement(dl,"Triangle1",V00="0",V01="1",V02="2")
                if len(points)==4:E.SubElement(dl,"Triangle1",V00="0",V01="2",V02="3")
                offset+=len(points)
            E.SubElement(dl,"EndDisplayList")
            resources[f"lighting/infill/{self.name}-{role}-vtx"]=E.tostring(vertex)
            resources[f"lighting/infill/{self.name}-{role}-dl"]=E.tostring(dl)
            E.SubElement(root,"CallDisplayList",Path=f"lighting/infill/{self.name}-{role}-dl")
        E.SubElement(root,"SetGeometryMode",G_LIGHTING="1")
        E.SubElement(root,"EndDisplayList")
        resources[f"lighting/infill/{self.name}"]=E.tostring(root)
        audit.append(dict(mesh=self.name,triangles=self.tris,
            low=[min(p[a] for p in self.bounds) for a in range(3)],
            high=[max(p[a] for p in self.bounds) for a in range(3)]))

town=Mesh("town")
# Outer blocks leave the existing alley, castle and Temple roads open.
for j,(z0,z1) in enumerate(((-2200,-1540),(-1510,-850),(-820,-160),(-130,530),(560,1220),(1250,1910))):
    town.building(-1830,-1250,z0,z1,400+(j%3)*45,160)
for j,(z0,z1) in enumerate(((-500,160),(190,850),(880,1540),(1570,2230))):
    town.building(1650,2230,z0,z1,430+(j%2)*60,170)
for z0,z1 in ((-2390,-2070),(2210,2580)):
    town.building(-1240,-230,z0,z1,420,180)
    town.building(230,1400,z0,z1,440,170)
# Lower ground catches views under the model's outer edges; do not cover the
# original market square, fountain, steps or their collision surfaces.
for x0,x1,z0,z1 in ((-2300,-600,-2450,2650),(1050,2500,-2450,2650),(-600,1050,750,2650),(-600,1050,-2450,-1900)):
    town.face("ground",[(x0,-100,z0),(x1,-100,z0),(x1,-100,z1),(x0,-100,z1)],
              ((x1-x0)/250,(z1-z0)/250),170)
# Castle approach continues far north of the earlier -2450 cutoff.
# Keep the playable street and transition at x=0 open, with complete outer
# blocks on both sides and a distant cross-street beyond the exit.
for j,(z0,z1) in enumerate(((-4210,-3550),(-3520,-2860),(-2830,-2170),(-2140,-1480),(-1450,-790),(-760,-510))):
    if j < 3:
        town.building(-1800,-1110,z0,min(z1,-2230),430+(j%2)*35,150)
    town.building(1120,1740,z0,z1,440+(j%3)*30,160)
town.building(-1100,-270,-4430,-4110,400,150)
town.building(270,1120,-4430,-4110,420,150)
# Lower terrain behind all scene edges stops views through to the sky below
# the town, without covering the original walking surface or changing collision.
town.face("ground",[(-5000,-105,-6000),(5000,-105,-6000),(5000,-105,5000),(-5000,-105,5000)],(24,24),180)
town.export()

temple=Mesh("temple-courtyard")
for j,(z0,z1) in enumerate(((-2050,-1450),(-1420,-900),(-870,-570),(-300,260))):
    temple.building(-380,215,z0,z1,300+(j%2)*55,125)
# The Market exit is at (793,60,-420), facing east on entry. Keep that westward
# view as an arch and descending street rather than closing it with a wall.
temple.box(185,225,245,315,-570,-300,"stone",180)
temple.face("ground",[(-1400,-6,-570),(540,-6,-570),(540,-6,-300),(-1400,-6,-300)],(7,1),180)
temple.building(-1250,-1000,-1100,-580,330,140)
temple.building(-1250,-1000,-290,300,350,140)
# Beyond the actual scene transition, show the opposite side of a cross street.
# This closes the skyline through the arch without blocking the playable exit.
temple.building(-1850,-1420,-1000,180,340,140)
temple.face("ground",[(-1420,-6,-1100),(-1000,-6,-1100),(-1000,-6,300),(-1420,-6,300)],(2,6),180)
# Close the garden's north/south skyline behind its existing fences and trees.
temple.box(210,3100,-100,240,-1990,-1870,"stone",180)
temple.box(210,3100,-100,240,120,240,"stone",180)
for z in (-1990,120):
    for x in range(240,3090,160):temple.box(x,x+75,240,280,z,z+120,"stone",180)
for x0,x1,z0,z1 in ((-1900,240,-2300,400),(240,3150,-2300,-1765),(240,3150,-15,400)):
    temple.face("ground",[(x0,-100,z0),(x1,-100,z0),(x1,-100,z1),(x0,-100,z1)],
                ((x1-x0)/250,(z1-z0)/250),170)
temple.export()

for folder in (ASSETS/"alt/scenes/shared").iterdir():
    if folder.name in ("market_day_scene","market_night_scene","market_ruins_scene"):mesh=town
    elif folder.name in ("shrine_scene","shrine_n_scene","shrine_r_scene"):mesh=temple
    else:continue
    for top in folder.glob("*room_0DL_*"):
        if any(k in top.name for k in ("_tri_","_vtx_")) or top.name.startswith("mat_"):continue
        if not top.read_bytes().startswith(b"<DisplayList"):continue
        tree=E.fromstring(top.read_bytes())
        end=tree.find("EndDisplayList");assert end is not None
        tree.insert(list(tree).index(end),E.Element("CallDisplayList",Path=f"lighting/infill/{mesh.name}"))
        resources[str(top.relative_to(ASSETS)).replace("\\","/")]=E.tostring(tree)
        for load in tree.findall("LoadVertices"):
            if "_vtx_cull" not in load.attrib["Path"]:continue
            p=ASSETS/"alt"/load.attrib["Path"]
            v=E.fromstring(p.read_bytes());xs=[int(a.attrib["X"]) for a in v];ys=[int(a.attrib["Y"]) for a in v];zs=[int(a.attrib["Z"]) for a in v]
            lo=[min(min(xs),min(a[0] for a in mesh.bounds)),min(min(ys),min(a[1] for a in mesh.bounds)),min(min(zs),min(a[2] for a in mesh.bounds))]
            hi=[max(max(xs),max(a[0] for a in mesh.bounds)),max(max(ys),max(a[1] for a in mesh.bounds)),max(max(zs),max(a[2] for a in mesh.bounds))]
            for a in v:
                for k,vals,mn,mx in zip("XYZ",(xs,ys,zs),lo,hi):a.attrib[k]=str(round(mn if int(a.attrib[k])==min(vals) else mx))
            resources["alt/"+load.attrib["Path"]]=E.tostring(v)

# The master-sword chamber's floor used vertex colors as a second fake sunlight
# layer. Keep its positions, UVs, alpha, textures and culling vertices, replacing
# only the baked floor irradiance with a neutral albedo shade for the real light.
floor_audit=[]
with zipfile.ZipFile(args.game_archive) as base:
    for suffix in ("004C68","0060B8","007400"):
        name=f"scenes/shared/tokinoma_scene/tokinoma_room_0Vtx_{suffix}"
        data=base.read(name)
        assert data[4:8]==b"RRAO"
        kind,count=struct.unpack_from("<II",data,64)
        assert kind==25 and len(data)==72+count*16
        vertex=E.Element("Vertex",Version="0");changed=0
        for i in range(count):
            x,y,z,flag,s,t,r,g,b,a=struct.unpack_from("<3hH2h4B",data,72+i*16)
            if a:
                assert r==g==b
                r=g=b=160;changed+=1
            E.SubElement(vertex,"Vtx",**dict(zip(("X","Y","Z","S","T","R","G","B","A"),map(str,(x,y,z,s,t,r,g,b,a)))))
        resources["alt/"+name]=E.tostring(vertex)
        floor_audit.append(dict(resource=name,vertices=count,recolored=changed))

# Real reverse faces for the existing opaque walls and roofs complement the
# closed outer blocks. Foliage, banners, glass and other cutouts are untouched.
import copy
backings=[]
for folder in (ASSETS/"alt/scenes/shared").iterdir():
    if not folder.name.startswith(("market_","shrine")):continue
    for top in folder.glob("*room_*DL_*"):
        if any(k in top.name for k in ("_tri_","_vtx_")) or top.name.startswith("mat_"):continue
        if not top.read_bytes().startswith(b"<DisplayList"):continue
        texture=""
        for call in E.fromstring(top.read_bytes()):
            if call.tag!="CallDisplayList":continue
            name=call.attrib["Path"]
            if "/mat_" in name:
                material=E.fromstring((ASSETS/"alt"/name).read_bytes())
                tex=material.find("SetTextureImage")
                texture=tex.attrib["Path"] if tex is not None else ""
                continue
            if not any(k in texture for k in ("kabe","yane","mado","siro","drwk")):continue
            path=ASSETS/"alt"/name
            if not path.exists():continue
            src=E.fromstring(path.read_bytes());dst=E.Element("DisplayList",src.attrib);added=0
            for cmd in src:
                dst.append(copy.deepcopy(cmd))
                if cmd.tag not in ("Triangle1","Triangles2"):continue
                back=copy.deepcopy(cmd)
                for prefix in ("V0","V1")[:1 if cmd.tag=="Triangle1" else 2]:
                    a,b=prefix+"1",prefix+"2"
                    back.attrib[a],back.attrib[b]=back.attrib[b],back.attrib[a];added+=1
                dst.append(back)
            if added:
                resources["alt/"+name]=E.tostring(dst)
                backings.append(dict(resource=name,triangles=added))
archive=Path(args.output)/"ZZ-LightingLab-Architecture.o2r"
with zipfile.ZipFile(archive,"w",zipfile.ZIP_DEFLATED) as z:
    for name,data in resources.items():z.writestr(name,data)
(Path(args.output)/"architecture-infill-audit.json").write_text(json.dumps(dict(meshes=audit,temple_floor=floor_audit,backings=backings,resources=list(resources)),indent=2))
print(json.dumps(dict(meshes=audit,resources=len(resources))))
