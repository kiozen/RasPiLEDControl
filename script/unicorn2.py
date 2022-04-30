import json
import colorsys

def Color(red, green, blue):
    return red<<16|green<<8|blue

def rotate(l, n):
    return l[-n:] + l[:-n]

j = {"data" : [], "mode": "cyclic", "name": "Unicorn2", "description" : "The tiny little unicorn shits a rainbow of colors all over your place."}

NUM_LED = 195
FRAME_TIME = 500

colors = [0]*NUM_LED

for i in range(300):
    r,g,b = colorsys.hsv_to_rgb(i/300.0, 1, 0.5)    
    colors = [Color(round(r*255), round(g*255), round(b*255))]*NUM_LED
    j["data"].append([FRAME_TIME, colors])
    
with open("unicorn2.json", "w") as fid:
    fid.write(json.dumps(j))
