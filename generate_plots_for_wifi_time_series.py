import sys

with open(sys.argv[1], "r") as file:
    with open(sys.argv[2], "w") as out_server:
        with open(sys.argv[3], "w") as out_client:
            for line in file:
                words = line.split(' ')
                time = float(words[2].rstrip('s'))
                if 'client=' in line:
                    out_client.write(str(time) + " 2\n")
                else:
                    out_server.write(str(time) + " 1\n")