import sys

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

def write_ranking(blocks, outfile):
    with open(outfile, 'w') as f:
        for i, size in enumerate(sorted(blocks, reverse=True), 1):
            f.write(f"{i} {size}\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python pgm_linear_255_blocks.py input.pgm output.txt")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    data = read_pgm_linear(input_path)
    blocks = find_linear_blocks(data)
    write_ranking(blocks, output_path)
