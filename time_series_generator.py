import sys

input_file_name = sys.argv[1]
output_file_name = sys.argv[2]

last_lost = ""

with open(input_file_name, 'r') as input_file:
    with open(output_file_name, 'w') as output_file:
        for line in input_file:
            if 'sent' in line:
                output_file.write(line)
            elif 'lost' in line:
                if last_lost != line:
                    output_file.write(line)
                    last_lost = line
