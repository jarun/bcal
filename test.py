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

import pytest
import subprocess

test = [
    ('./bcal', '-m', '10', 'mb'),                                      # 0
    ('./bcal', '-m', '10', 'TiB'),                                     # 1
    ('./bcal', '-m', '.1', '   KIb    '),                              # 2
    ('./bcal', '-m', '.1', 'KB  '),                                    # 3
    ('./bcal', '-m', '10', 'lb'),                                      # 4
    ('./bcal', '-m', "0x4         kb   *  2     +   5        mib"),    # 5
    ('./bcal', '-m', "5*5*5*5     mIB"),                               # 6
    ('./bcal', '-m', "5mb*5*5*5"),                                     # 7
    ('./bcal', '-m', "5 tb / 12"),                                     # 8
    ('./bcal', '-m', "2kb+3mb/4*5+5*56mb"),                            # 9
    ('./bcal', '-m', "( 5 * 3) * (4 * 7 b)"),                          # 10
    ('./bcal', '-m', "( 5 * 3 + 8) * (4 * 7 b)"),                      # 11
    ('./bcal', '-m', "( 5 * (3 + 8 )) * (4 * 7 b )"),                  # 12
    ('./bcal', '-m', "( 5 ) * 2 mib"),                                 # 13
    ('./bcal', '-m', "3   mb -  2    mib"),                            # 14
    ('./bcal', '-m', "2mb-3mib"),                                      # 15
    ('./bcal', '-m', "2 mib * -2"),                                    # 16
    ('./bcal', '-m', "2mib*-2"),                                       # 17
    ('./bcal', '-m', "2giB*2/2"),                                      # 18
    ('./bcal', '-m', "1miB / 4 kib"),                                  # 19
    ('./bcal', '-m', "(2giB*2)/2kib"),                                 # 20
    ('./bcal', '-m', "1b / 0"),                                        # 21
    ('./bcal', '-m', "2qB*2"),                                         # 22
    ('./bcal', '-m', "((2giB)*2/2)"),                                  # 23
    ('./bcal', '-m', "((2giB)*(2/2)"),                                 # 24
    ('./bcal', '-m', "((2giB)*1)/(2/2))"),                             # 25
    ('./bcal', '-m', "((2giB)*1)/(2/2)"),                              # 26
    ('./bcal', '-m', "(((2giB)*)2/2)"),                                # 27
    ('./bcal', '-m', "(2giB)*2*"),                                     # 28
    ('./bcal', '-m', '0b11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111', 'b'),  # 29
    ('./bcal', '-m', '0xffffffffffffffffffffffffffffffff', 'b'),       # 30
    ('./bcal', '-m', '340282366920938463463374607431768211455', 'b'),  # 31
    ('./bcal', '-m', '0xffffffffffffffff', 'b'),                       # 32
    ('./bcal', '-m', "2b / 3"),                                        # 33
    ('./bcal', '-m', "2 kIb/((3 ) )"),                                 # 34
    ('./bcal', '-m', "2 gIb/ - 3"),                                    # 35
    ('./bcal', '-m', "(2) kIb/((3))"),                                 # 36
    ('./bcal', '-m', "(2) 4 kIb/((3))"),                               # 37
    ('./bcal', '-m', "(2) 4 kIb/((3))(2)"),                            # 38
    ('./bcal', '-m', "2 / 3 tib   "),                                  # 39
    ('./bcal', '-m', " 1000 "),                                        # 40
    ('./bcal', '-m', " 0x1234mib  "),                                  # 41
    ('./bcal', '-m', "        "),                                      # 42
    ('./bcal', '-m', "0x18mb"),                                        # 43
    ('./bcal', '-m', "0x18mb", "kb"),                                  # 44
    ('./bcal', '-m', "0x18mb82"),                                      # 45
    ('./bcal', '-m', "0x18mbc4"),                                      # 46
    ('./bcal', '-m', "0x18mb 82"),                                     # 47
    ('./bcal', '-m', "0x18mb", "82"),                                  # 48

    ('./bcal', "-c", "0b"),                                            # 49
    ('./bcal', "-c", "0x"),                                            # 50
    ('./bcal', "-c", "0"),                                             # 51
    ('./bcal', "-c", "0b11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"),   # 52
    ('./bcal', "-c", "0xffffffffffffffffffffffffffffffff"),            # 53
    ('./bcal', "-c", "340282366920938463463374607431768211455"),       # 54
    ('./bcal', "-c", "0b1111111111111111111111111111111111111111111111111111111111111111"),                                                                   # 55
    ('./bcal', "-c", "0b111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"),  # 56
    ('./bcal', '-m', "1kib/ 4kb"),                                     # 57
    ('./bcal', '-m', "0kib /4kb"),                                     # 58
    ('./bcal', '-m', "{10+2"),                                         # 59
    ('./bcal', '-m', "10 + 2]"),                                       # 60
    ('./bcal', '-m', "2 >>> 2"),                                       # 61
    ('./bcal', '-m', "2 b<3"),                                         # 62
    ('./bcal', '-m', "(2giB * 2) / (2kib >> 2)"),                      # 63
    ('./bcal', '-b', "9876543210.987654321 * 123456789.123456789"),    # 64
    ('./bcal', "4 + 3 - 2 + 10 * 5 / 2 + 7 - 6 + 9 - 10 / 5 * 2"),     # 65
    ('./bcal', '-m', "4b + 3b - 2b + 10b * 5 / 2 + 7b - 6b + 9b - 10b / 5 * 2"),  # 66
    ('./bcal', '-m', "0xff / 0xf + 1337"),                             # 67
    ('./bcal', '-m', "(0xff giB * 2) / (2kib >> 2)"),                  # 68
    ('./bcal', '-m', "(2.2giB * 2) / (2.2kib >> 2)"),                  # 69
    ('./bcal', '-m', "0xbb b * 2"),                                    # 70
    ('./bcal', '-m', "0xbb * 2"),                                      # 71
    ('./bcal', '-b', "(50,000 - 2,000) * 1,500"),                      # 72
    ('./bcal', '-m', "5 & 3"),                                         # 73
    ('./bcal', '-m', "5 | 2"),                                         # 74
    ('./bcal', '-m', "5 ^ 3"),                                         # 75
    ('./bcal', '-m', "~0xff & 0xff"),                                  # 76
    ('./bcal', '-m', "1 << 8 >> 4"),                                   # 77
    ('./bcal', '-m', "1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 1024"),  # 78
    ('./bcal', '-m', "1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1"),    # 79
]

res = [
    b'10000000 B\n',                                 # 0
    b'10995116277760 B\n',                           # 1
    b'102 B\n',                                      # 2
    b'100 B\n',                                      # 3
    b'ERROR: unknown unit\n',                        # 4
    b'5250880 B\n',                                  # 5
    b'655360000 B\n',                                # 6
    b'625000000 B\n',                                # 7
    b'WARNING: result truncated\n416666666666 B\n',  # 8
    b'283752000 B\n',                                # 9
    b'420 B\n',                                      # 10
    b'644 B\n',                                      # 11
    b'1540 B\n',                                     # 12
    b'10485760 B\n',                                 # 13
    b'902848 B\n',                                   # 14
    b'ERROR: negative result\n',                     # 15
    b'ERROR: negative token\n',                      # 16
    b'ERROR: negative token\n',                      # 17
    b'2147483648 B\n',                               # 18
    b'256\n',                                        # 19
    b'2097152\n',                                    # 20
    b'ERROR: division by 0\n',                       # 21
    b'ERROR: unknown unit\n',                        # 22
    b'2147483648 B\n',                               # 23
    b'ERROR: unbalanced expression\n',               # 24
    b'ERROR: unbalanced expression\n',               # 25
    b'2147483648 B\n',                               # 26
    b'ERROR: invalid token\n',                       # 27
    b'ERROR: invalid token\n',                       # 28
    b'340282366920938463463374607431768211455 B\n',  # 29
    b'340282366920938463463374607431768211455 B\n',  # 30
    b'340282366920938463463374607431768211455 B\n',  # 31
    b'18446744073709551615 B\n',                     # 32
    b'WARNING: result truncated\n0 B\n',             # 33
    b'WARNING: result truncated\n682 B\n',           # 34
    b'ERROR: negative token\n',                      # 35
    b'ERROR: invalid expression\n',                  # 36
    b'WARNING: result truncated\nERROR: invalid expression\n',  # 37
    b'WARNING: result truncated\nERROR: invalid expression\n',  # 38
    b'ERROR: unit mismatch in /\n',                  # 39
    b'1000 B\n',                                     # 40
    b'4886364160 B\n',                               # 41
    b'ERROR: invalid value\n',                       # 42
    b'24000000 B\n',                                 # 43
    b'ERROR: malformed input\n',                     # 44
    b'ERROR: malformed input\n',                     # 45
    b'ERROR: malformed input\n',                     # 46
    b'ERROR: malformed input\n',                     # 47
    b'ERROR: unknown unit\n',                        # 48

    b'ERROR: invalid input\n\n',                     # 49
    b'ERROR: invalid input\n\n',                     # 50
    b' (b) 0\n (d) 0\n (h) 0x0\n\n',                 # 51
    b' (b) 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111\n (d) 340282366920938463463374607431768211455\n (h) 0xffffffffffffffffffffffffffffffff\n\n',  # 52
    b' (b) 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111\n (d) 340282366920938463463374607431768211455\n (h) 0xffffffffffffffffffffffffffffffff\n\n',  # 53
    b' (b) 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111\n (d) 340282366920938463463374607431768211455\n (h) 0xffffffffffffffffffffffffffffffff\n\n',  # 54
    b' (b) 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111\n (d) 18446744073709551615\n (h) 0xffffffffffffffff\n\n',  # 55
    b'ERROR: invalid input\n\n',                     # 56
    b'WARNING: result truncated\n0\n',               # 57
    b'0\n',                                          # 58
    b'ERROR: first brackets only\n',                 # 59
    b'ERROR: first brackets only\n',                 # 60
    b'ERROR: invalid sequence >>>\n',                # 61
    b'ERROR: invalid operator <\n',                  # 62
    b'8388608\n',                                    # 63
    b'1219326312467611632.3609205901\n',             # 64
    b'36\n',                                         # 65
    b'36 B\n',                                       # 66
    b'1354\n',                                       # 67
    b'1069547520\n',                                 # 68
    b'WARNING: result truncated\n8391587\n',         # 69
    b'374 B\n',                                      # 70
    b'374\n',                                        # 71
    b'72000000\n',                                   # 72
    b'1\n',                                          # 73
    b'7\n',                                          # 74
    b'6\n',                                          # 75
    b'0\n',                                          # 76
    b'16\n',                                         # 77
    b'2047\n',                                       # 78
    b'1024\n',                                       # 79
]


@pytest.mark.parametrize('item, res', zip(test, res))
def test_output(item, res):
    try:
        out = subprocess.check_output(item, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        # print(e.output)
        assert e.output == res
    else:
        assert out == res
