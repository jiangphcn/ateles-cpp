#!/usr/bin/env python

import sys


def make_bytes(src):
    for line in src:
        for c in line:
            yield "0x{:02x}".format(ord(c))


def mkheader(base_varname, src, tgt):
    tgt.write("unsigned char %s_data[] = {\n" % base_varname)
    total_size = 0
    curr_row = 0
    for byte in make_bytes(src):
        if total_size > 0 and curr_row != 12:
            tgt.write(",")
        elif total_size > 0:
            tgt.write(",")
        if curr_row == 12:
            tgt.write("\n")
            curr_row = 0
        tgt.write(byte)
        total_size += 1
        curr_row += 1
    if curr_row != 12:
        tgt.write("\n")
    tgt.write("};\n")
    tgt.write("unsigned int %s_len = %d;\n" % (base_varname, total_size))


def main():
    if len(sys.argv) != 4:
        print "usage: %s infile outfile base_varname" % sys.argv[0]
        exit(1)

    with open(sys.argv[1]) as src:
        with open(sys.argv[2], "wb") as tgt:
            mkheader(sys.argv[3], src, tgt)


if __name__ == "__main__":
    main()
