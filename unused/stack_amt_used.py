import os, re

os.system("cd ../; make asm")

with open("../bin/dungeon_dave.asm", "r") as asm_file:
	rows = asm_file.readlines()

largest_num = -1
row_with_largest_num = ""
line_num_with_largest_num = -1

for i, row in enumerate([r.strip() for r in rows]):
	if "[rbp" in row or "[rsp" in row:
		for match in [int(m) for m in re.findall("[0-9]+", row)]:
			if match > largest_num:
				largest_num = match
				row_with_largest_num = row
				line_num_with_largest_num = i + 1

print(f"The largest stack offset is {largest_num}, on row #{line_num_with_largest_num}:")
print(f"`{row_with_largest_num}`")