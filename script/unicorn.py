import json
import colorsys

def Color(red, green, blue):
    return red<<16|green<<8|blue

def rotate(l, n):
    return l[-n:] + l[:-n]

j = {"data" : [], "mode": "cyclic", "name": "Unicorn", "description" : "The tiny little unicorn pukes a fantastic revolving rainbow for you."}

NUM_LED = 300
FRAME_TIME = 100

colors = [0]*NUM_LED

for i in range(NUM_LED):
    r,g,b = colorsys.hsv_to_rgb(i/NUM_LED, 1, 0.5)    
    colors[i] = Color(round(r*255), round(g*255), round(b*255));
    

for i in range(NUM_LED):
    colors = rotate(colors, 1)
    j["data"].append([FRAME_TIME, colors])


with open("unicorn.json", "w") as fid:
    fid.write(json.dumps(j))
