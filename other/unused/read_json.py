import json

with open("engine.json", "r") as engine_file:
	contents = json.load(engine_file)
	print(contents)