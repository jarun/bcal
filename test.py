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
    ('./bcal', '-m', '.1', '   KIb    '),                            # 3
    ('./bcal', '-m', '.1', 'KB  '),                                  # 4
    ('./bcal', '-m', '10', 'lb'),                                    # 5
    ('./bcal', '-m', "0x4         kb   *  2     +   5        mib"),  # 6
    ('./bcal', '-m', "5*5*5*5     mIB"),                             # 7
    ('./bcal', '-m', "5mb*5*5*5"),                                   # 8
    ('./bcal', '-m', "5 tb / 12"),                                   # 9
    ('./bcal', '-m', "2kb+3mb/4*5+5*56mb"),                          # 10
    ('./bcal', '-m', "( 5 * 3) * (4 * 7 b)"),                        # 11
    ('./bcal', '-m', "( 5 * 3 + 8) * (4 * 7 b)"),                    # 12
    ('./bcal', '-m', "( 5 * (3 + 8 )) * (4 * 7 b )"),                # 13
    ('./bcal', '-m', "( 5 ) * 2 mib"),                               # 14
    ('./bcal', '-m', "3   mb -  2    mib"),                          # 15
    ('./bcal', '-m', "2mb-3mib"),                                    # 16
    ('./bcal', '-m', "2 mib * -2"),                                  # 17
    ('./bcal', '-m', "2mib*-2"),                                     # 18
    ('./bcal', '-m', "2giB*2/2"),                                    # 19
    ('./bcal', '-m', "2qB*2"),                                       # 20
    ('./bcal', '-m', "((2giB)*2/2)"),                                # 21
    ('./bcal', '-m', "((2giB)*(2/2)"),                               # 22
    ('./bcal', '-m', "((2giB)*1)/(2/2))"),                           # 23
    ('./bcal', '-m', "((2giB)*1)/(2/2)"),                            # 24
    ('./bcal', '-m', "(((2giB)*)2/2)"),                              # 25
    ('./bcal', '-m', '0xffffffffffffffffffffffffffffffff', 'b'),     # 26
    ('./bcal', '-m', "2b / 3"),                                      # 27
    ('./bcal', '-m', "2 kIb/((3 ) )"),                               # 28
    ('./bcal', '-m', "2 gIb/ - 3"),                                  # 29
    ('./bcal', '-m', "(2) kIb/((3))"),                               # 30
    ('./bcal', '-m', "(2) 4 kIb/((3))"),                             # 31
    ('./bcal', '-m', "(2) 4 kIb/((3))(2)"),                          # 32
    ('./bcal', '-m', "2 / 3 tib   "),                                # 33
    ('./bcal', '-m', " 1000 "),                                      # 34
    ('./bcal', '-m', " 0x1234mib  "),                                # 35
    ('./bcal', '-m', "        "),                                    # 36
    ('./bcal', '-m', "0x18mb"),                                      # 37
    ('./bcal', '-m', "0x18mb", "kb"),                                # 38
    ('./bcal', '-m', "0x18mb82"),                                    # 39
    ('./bcal', '-m', "0x18mbc4"),                                    # 40
    ('./bcal', '-m', "0x18mb 82"),                                   # 41
    ('./bcal', '-m', "0x18mb", "82"),                                # 42
]

res = [
    b'10000000\n',                                 # 1
    b'10995116277760\n',                           # 2
    b'102\n',                                      # 3
    b'100\n',                                      # 4
    b'ERROR: unknown unit\n',                      # 5
    b'5250880\n',                                  # 6
    b'655360000\n',                                # 7
    b'625000000\n',                                # 8
    b'416666666666\n',                             # 9
    b'283752000\n',                                # 10
    b'420\n',                                      # 11
    b'644\n',                                      # 12
    b'1540\n',                                     # 13
    b'10485760\n',                                 # 14
    b'902848\n',                                   # 15
    b'ERROR: negative result\n',                   # 16
    b'ERROR: negative token\n',                    # 17
    b'ERROR: negative token\n',                    # 18
    b'2147483648\n',                               # 19
    b'ERROR: unknown unit\n',                      # 20
    b'2147483648\n',                               # 21
    b'ERROR: unbalanced expression\n',             # 22
    b'ERROR: unbalanced expression\n',             # 23
    b'2147483648\n',                               # 24
    b'ERROR: invalid token\n',                     # 25
    b'340282366920938463463374607431768211455\n',  # 26
    b'0\n',                                        # 27
    b'682\n',                                      # 28
    b'ERROR: negative token\n',                    # 29
    b'ERROR: invalid expression\n',                # 30
    b'ERROR: invalid expression\n',                # 31
    b'ERROR: invalid expression\n',                # 32
    b'ERROR: unit mismatch in /\n',                # 33
    b'1000\n',                                     # 34
    b'4886364160\n',                               # 35
    b'ERROR: invalid value\n',                     # 36
    b'24000000\n',                                 # 37
    b'ERROR: malformed input\n',                   # 38
    b'ERROR: malformed input\n',                   # 39
    b'ERROR: malformed input\n',                   # 40
    b'ERROR: malformed input\n',                   # 41
    b'ERROR: unknown unit\n',                      # 42
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
