import json

i = input("Enter filename: ")
f = open(i, "r")
data = json.load(f)

print("{", end="")
for line in data:
    for i in line:
        if (i == "#000000"):
            print("1, ", end="")
        elif (i == "#ff0000"):
            print("2, ", end="")
        else:
            print("0, ", end="")

print("}")