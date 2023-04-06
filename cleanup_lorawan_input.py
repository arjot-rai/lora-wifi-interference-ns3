import sys

if len(sys.argv) != 3:
    print("Usage: python remove_lines_without_sent.py input_file output_file")
    sys.exit(1)

input_file_name = sys.argv[1]
output_file_name = sys.argv[2]

with open(input_file_name, 'r') as input_file:
    with open(output_file_name, 'w') as output_file:    
        for line in input_file:
            if 'client=' in line or 'server=' in line:
                output_file.write(line)

