#!/usr/bin/python3
import json
import matplotlib as mpl
import numpy as np

def colorFader(c1,c2,mix=0): #fade (linear interpolate) from color c1 (at mix=0) to c2 (mix=1)
    c1=np.array(mpl.colors.to_rgb(c1))
    c2=np.array(mpl.colors.to_rgb(c2))
    return mpl.colors.to_rgb((1-mix)*c1 + mix*c2)


j = {"data" : [], "name": "Magic Sunrise", "description" : "Start your morning with a beautiful artificial sunrise while the rest of the world is still in the dark."}

total_time = 0
c1 = "#030100"
c2 = "#783010"
n = 50
for x in range(n):
    red,green,blue = colorFader(c1,c2,x/n)    
    color = int(round(red*255))<<16|int(round(green*255))<<8|int(round(blue*255))
    for k in range(0, 300, 5):
        j["data"].append([300, [color]*k])
        total_time += 300


#j["data"].append([600000, [color]*300])

print(f"total time {total_time/60000} minutes")

with open("sunrise.json", "w") as fid:
    fid.write(json.dumps(j))
