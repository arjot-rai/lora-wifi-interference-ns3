import sys

with open(sys.argv[1], 'r') as input_file:
    for line in input_file:
        l = line[25:]
        l = l.split(":")
        print("(" + l[0] + ", " + l[1] + ") [b]")
