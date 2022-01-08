#!/usr/bin/python3
import json


j = {"data" : [], "name": "Example", "description" : "Just an example as proof of concept."}

for t in range(0,100):
    j["data"].append([100, [(0)<<24|(0)<<16|(t)<<8|t]*300])

with open("example.json", "w") as fid:
    fid.write(json.dumps(j))

