import json
import random

NUM_LED = 300
FRAME_TIME = 150

def Color(red, green, blue):
    return red<<16|green<<8|blue

j = {"data" : [], "mode": "cyclic", "name": "Fireplace", "description" : "Make yourself confortable in front of non-existing sizzling fireplace."}

actColors = [[20,0,0]] * NUM_LED

numOfNewValues = 8

colors = [0] *NUM_LED
for i in range(NUM_LED):
    colors[i] = Color(actColors[i][0], actColors[i][1], actColors[i][2])
j["data"].append([FRAME_TIME, colors])

for x in range(2000):
    for i in range(numOfNewValues):
        index = random.randrange(NUM_LED)
        actColors[index][0] = min(actColors[index][0] + 100, 255)
        actColors[index][1] = min(actColors[index][1] + 60,255)

    oldColors = actColors
    actColors = [[0,0,0]] * NUM_LED
    for i in range(NUM_LED):
        act = oldColors[i]
        right = oldColors[(i+1) % NUM_LED]
        left = oldColors[(i-1) % NUM_LED]
        actColors[i] = [max(right[0]//6 + act[0]*2//3 + left[0]//6 - 2,0),max(right[1]//6 + act[1]*2//3 + left[1]//6 - 5, 0), 0]

    if x < 1000:
        continue

    colors = [0] *NUM_LED
    for i in range(NUM_LED):
        colors[i] = Color(actColors[i][0], actColors[i][1], actColors[i][2])
    j["data"].append([FRAME_TIME, colors])


with open("fireplace.json", "w") as fid:
    fid.write(json.dumps(j))

# l = strip.numPixels()
# actColors = [[0,0,0]] * l
# for i in range(l):
#     actColors[i] = [20,0,0]
# numOfNewValues = 8
# for i in range(l):
#     strip.setPixelColor(i, Color(actColors[i][0], actColors[i][1], actColors[i][2]))
# strip.show()
# time.sleep(self.wait_ms/1000.0)
# while True:
#     for i in range(numOfNewValues):
#         index = random.randrange(l)
#         actColors[index][0] = min(actColors[index][0] + 100, 255)
#         actColors[index][1] = min(actColors[index][1] + 60,255)
       
#     oldColors = actColors
#     actColors = [[0,0,0]] * l
#     for i in range(l):
#         act = oldColors[i]
#         right = oldColors[(i+1) % l]
#         left = oldColors[(i-1) % l]
#         actColors[i] = [max(right[0]//6 + act[0]*2//3 + left[0]//6 - 2,0),max(right[1]//6 + act[1]*2//3 + left[1]//6 - 5, 0), 0]
    
#     for i in range(l):
#         strip.setPixelColor(i, Color(actColors[i][0], actColors[i][1], actColors[i][2]))
#     strip.show()
#     time.sleep(self.wait_ms/1000.0)