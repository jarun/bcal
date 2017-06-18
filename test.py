#!/usr/bin/env python3

'''
bcal (Byte CALculator) test script

Author: Arun Prakash Jana
Email : engineerarun@gmail.com
Home  : https://github.com/jarun/bcal

NOTES:

1. Use the `-l` (light mode) option to print and
   verify against minimal output in decimal bytes

2. Before raising a PR,
   a. add relevant test cases
   b. run `make test`
'''

import subprocess

cases = [
    ('./bcal', '-l', '10', 'mb'),                                    # 1
    ('./bcal', '-l', '10', 'mib'),                                   # 2
    ('./bcal', '-l', "0x4         kb   *  2     +   5        mib"),  # 3
    ('./bcal', '-l', "5*5*5*5mib"),                                  # 4
    ('./bcal', '-l', "5mb*5*5*5"),                                   # 5
    ('./bcal', '-l', "5 tb / 12"),                                   # 6
    ('./bcal', '-l', "2kb+3mb/4*5+5*56mb"),                          # 7
    ('./bcal', '-l', "( 5 * 3) * (4 * 7 b)"),                        # 8
    ('./bcal', '-l', "( 5 * 3 + 8) * (4 * 7 b)"),                    # 9
    ('./bcal', '-l', "( 5 * (3 + 8 )) * (4 * 7 b )"),                # 10
    ('./bcal', '-l', "( 5 ) * 2 mib"),                               # 11
    ('./bcal', '-l', "3   mb -  2    mib"),                          # 12
    ('./bcal', '-l', "2mb-3mib"),                                    # 13
]

results = [
    b'10000000\n',                                # 1
    b'10485760\n',                                # 2
    b'5250880\n',                                 # 3
    b'655360000\n',                               # 4
    b'625000000\n',                               # 5
    b'416666666666\n',                            # 6
    b'283752000\n',                               # 7
    b'420\n',                                     # 8
    b'644\n',                                     # 9
    b'1540\n',                                    # 10
    b'10485760\n',                                # 11
    b'902848\n',                                  # 12
    b'eval(), ERROR: Negative result\n',          # 13
]

print()

for index, item in enumerate(cases):
    print('Executing test %d' % (int)(index + 1))

    try:
        out = subprocess.check_output(item, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        assert e.output == results[index]
    else:
        assert out == results[index]

print('\nAll good! :)')
