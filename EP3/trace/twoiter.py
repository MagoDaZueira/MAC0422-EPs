import sys
from math import floor

def read_pgm_linear(filepath):
    with open(filepath, 'r') as f:
        lines = [line.strip() for line in f if not line.startswith('#') and line.strip()]
        assert lines[0] == 'P2', "Only plain PGM (P2) format supported"
        width, height = map(int, lines[1].split())
        maxval = int(lines[2])
        data = list(map(int, ' '.join(lines[3:]).split()))
        return data

def find_linear_blocks(data):
    blocks = []
    count = 0
    for val in data:
        if val == 255:
            count += 1
        else:
            if count > 0:
                blocks.append(count)
                count = 0
    if count > 0:
        blocks.append(count)
    return blocks

def divide(blocks, percentage):
    new_blocks = []
    for val in blocks:
        new_blocks.append(floor(val * percentage))
    for old, new in zip(blocks, new_blocks):
        if old - new != 0:
            new_blocks.append(old - new)
    return new_blocks

def write_file(blocks, outfile):
    with open(outfile, 'w') as f:
        for i, size in enumerate(blocks, 1):
            f.write(f"{i} {size}\n")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python pgm_linear_255_blocks.py percentage input.pgm output.txt")
        sys.exit(1)

    percentage = float(sys.argv[1])
    input_path = sys.argv[2]
    output_path = sys.argv[3]

    data = read_pgm_linear(input_path)
    blocks = find_linear_blocks(data)
    blocks = divide(blocks, percentage)
    write_file(blocks, output_path)
