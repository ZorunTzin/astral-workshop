"""Straight Keystone stems with clockwise four-node specialization branches.

Pure asset-generation geometry. Stable node IDs match existing saved Sigils;
there is no routing, layout search, or new work in the game's render loop.
"""
import bisect
import math

CENTER = (735,391)
ELLIPSE = (655,315)
RETIRED = 108


def diameter(uid):
    if uid < 108 and uid % 12 >= 9:
        return 20
    return 16 if uid >= 112 and (uid-112) % 4 == 3 else 14


def build_layout(extensions):
    angles=[-math.pi/2+math.tau*j/12000 for j in range(12001)]
    arc=[0.0]
    for a,b in zip(angles,angles[1:]):
        arc.append(arc[-1]+math.hypot(655*(math.cos(b)-math.cos(a)),315*(math.sin(b)-math.sin(a))))

    def boundary(spoke):
        distance=spoke/28*arc[-1]
        j=bisect.bisect_left(arc,distance);previous=max(0,j-1)
        angle=angles[previous]+(angles[j]-angles[previous])*(distance-arc[previous])/(arc[j]-arc[previous] or 1)
        return (655*math.cos(angle),315*math.sin(angle))

    ranks={(p,i,rank):uid for uid,(p,i,rank,_) in extensions.items()}
    positions={};edges=[];parents=[65534]*112;stems=[];labels={}
    for region in range(9):
        for path in range(3):
            # Four-node Legion stems occupy the space formerly divided among
            # four independent spokes. Every branch now ends at a Keystone.
            spoke=(18,19.5,20.75)[path] if region==6 else region*3+path+(region>6)
            dx,dy=boundary(spoke);radius=math.hypot(dx,dy)
            ux,uy=dx/radius,dy/radius;vx,vy=-uy,ux
            nodes=[path*3,path*3+2,path*3+1]+([13+path] if region==6 else [])
            ids=[12*region+i if i<12 else 108+i-12 for i in nodes]

            def point(distance,offset=0):
                return (round(735+ux*distance+vx*offset),round(391+uy*distance+vy*offset))

            for index,(node,uid) in enumerate(zip(nodes,ids)):
                positions[uid]=point((.29+.45*index/(len(nodes)-1))*radius)
                parents[uid]=65535 if not index else nodes[index-1]
                if index:edges.append((ids[index-1],uid,region,False))
                previous=uid
                for rank in range(1,5):
                    tail=ranks[region,node,rank]
                    # Three/four parallel compact chains fan clockwise and
                    # outward, matching the user's straight-stem sketch.
                    positions[tail]=point(.74*radius+(index+rank-1)*18.5,(len(nodes)-index)*17.5)
                    edges.append((previous,tail,region,True));previous=tail
            key=12*region+9+path
            positions[key]=point(1.07*radius);parents[key]=nodes[-1]
            edges.append((ids[-1],key,region,False));stems.append((*ids,key))
        label_spoke=(18+19.5+20.75)/3 if region==6 else region*3+1+(region>6)
        lx,ly=boundary(label_spoke)
        labels[region]=(round(735+lx*.19),round(391+ly*.19))

    assert len(positions)==447 and len(edges)==420 and len(stems)==27
    assert sum(not branch for _,_,_,branch in edges)==84
    assert parents[RETIRED]==65534
    for uid,(x,y) in positions.items():
        assert 20<x<1450 and 35<y<735,("Node outside tree",uid,x,y)
        for other,xy in positions.items():
            if other<=uid:continue
            gap=math.dist((x,y),xy)-(diameter(uid)+diameter(other))/2
            assert gap>=2,("Overlapping node targets",uid,other,gap)
    for start,end,_,_ in edges:
        a,b=positions[start],positions[end]
        dx,dy=b[0]-a[0],b[1]-a[1];length=dx*dx+dy*dy
        for uid,(x,y) in positions.items():
            if uid in (start,end):continue
            t=max(0,min(1,((x-a[0])*dx+(y-a[1])*dy)/length))
            distance=math.hypot(x-a[0]-t*dx,y-a[1]-t*dy)
            assert distance>=diameter(uid)/2+1,("Path crosses foreign node",start,end,uid)
    def orientation(a,b,c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    for n,(a,b,_,_) in enumerate(edges):
        for c,d,_,_ in edges[n+1:]:
            if len({a,b,c,d})<4:continue
            ab=orientation(positions[a],positions[b],positions[c])*orientation(positions[a],positions[b],positions[d])
            cd=orientation(positions[c],positions[d],positions[a])*orientation(positions[c],positions[d],positions[b])
            assert not (ab<0 and cd<0),("Crossing branches",a,b,c,d)
    return positions,edges,parents,labels,stems
