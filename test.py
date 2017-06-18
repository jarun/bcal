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

test = [
    ('./bcal', '-m', '10', 'mb'),                                    # 1
    ('./bcal', '-m', '10', 'TiB'),                                   # 2
    ('./bcal', '-m', '10', 'lb'),                                    # 3
    ('./bcal', '-m', "0x4         kb   *  2     +   5        mib"),  # 4
    ('./bcal', '-m', "5*5*5*5mib"),                                  # 5
    ('./bcal', '-m', "5mb*5*5*5"),                                   # 6
    ('./bcal', '-m', "5 tb / 12"),                                   # 7
    ('./bcal', '-m', "2kb+3mb/4*5+5*56mb"),                          # 8
    ('./bcal', '-m', "( 5 * 3) * (4 * 7 b)"),                        # 9
    ('./bcal', '-m', "( 5 * 3 + 8) * (4 * 7 b)"),                    # 10
    ('./bcal', '-m', "( 5 * (3 + 8 )) * (4 * 7 b )"),                # 11
    ('./bcal', '-m', "( 5 ) * 2 mib"),                               # 12
    ('./bcal', '-m', "3   mb -  2    mib"),                          # 13
    ('./bcal', '-m', "2mb-3mib"),                                    # 14
    ('./bcal', '-m', "2 mib * -2"),                                  # 15
    ('./bcal', '-m', "2mib*-2"),                                     # 16
    ('./bcal', '-m', "2giB*2/2"),                                    # 17
    ('./bcal', '-m', "2qB*2"),                                       # 18
    ('./bcal', '-m', "((2giB)*2/2)"),                                # 19
    ('./bcal', '-m', "((2giB)*(2/2)"),                               # 20
    ('./bcal', '-m', "((2giB)*1)/(2/2))"),                           # 21
    ('./bcal', '-m', "((2giB)*1)/(2/2)"),                            # 22
    ('./bcal', '-m', "(((2giB)*)2/2)"),                              # 23
    ('./bcal', '-m', '0xffffffffffffffffffffffffffffffff', 'b'),     # 24
    ('./bcal', '-m', "2b / 3"),                                      # 25
    ('./bcal', '-m', "2 kIb/((3 ) )"),                               # 26
    ('./bcal', '-m', "2 gIb/ - 3"),                                  # 27
    ('./bcal', '-m', "(2) kIb/((3))"),                               # 28
    ('./bcal', '-m', "(2) 4 kIb/((3))"),                             # 29
    ('./bcal', '-m', "(2) 4 kIb/((3))(2)"),                          # 30
]

res = [
    b'10000000\n',                                 # 1
    b'10995116277760\n',                           # 2
    b'ERROR: unknown unit\n',                      # 3
    b'5250880\n',                                  # 4
    b'655360000\n',                                # 5
    b'625000000\n',                                # 6
    b'416666666666\n',                             # 7
    b'283752000\n',                                # 8
    b'420\n',                                      # 9
    b'644\n',                                      # 10
    b'1540\n',                                     # 11
    b'10485760\n',                                 # 12
    b'902848\n',                                   # 13
    b'ERROR: negative result\n',                   # 14
    b'ERROR: negative token\n',                    # 15
    b'ERROR: negative token\n',                    # 16
    b'2147483648\n',                               # 17
    b'ERROR: unknown unit\n',                      # 18
    b'2147483648\n',                               # 19
    b'ERROR: unbalanced expression\n',             # 20
    b'ERROR: unbalanced expression\n',             # 21
    b'2147483648\n',                               # 22
    b'ERROR: invalid token\n',                     # 23
    b'340282366920938463463374607431768211455\n',  # 24
    b'0\n',                                        # 25
    b'682\n',                                      # 26
    b'ERROR: negative token\n',                    # 27
    b'ERROR: invalid expression\n',                # 28
    b'ERROR: invalid expression\n',                # 29
    b'ERROR: invalid expression\n',                # 30
]

print()

for index, item in enumerate(test):
    print('Executing test %d' % (int)(index + 1))

    try:
        out = subprocess.check_output(item, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        # print(e.output)
        assert e.output == res[index]
    else:
        assert out == res[index]

print('\nAll good! :)')
