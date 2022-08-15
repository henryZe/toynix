#
# Copyright (C) 2018 Henry.Zeng <henryz_e@163.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the MIT License.

import sys, os

def main(argv):

    size = os.path.getsize(argv[0])
    if size > 510:
        print("boot block too large: {} bytes (max 510)".format(size))
        return -1

    print("boot block is {} bytes (max 510)".format(size))

    with open(argv[0], 'ab+') as f:
        while f.tell() < 510:
            f.write(b'\0')
        f.write(b'\x55\xaa')

    return 0

if __name__ == "__main__":
    main(sys.argv[1:])
