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
    ('./bcal', '-p', '0xffffffffffffffffffffffffffffffff'),            # 57 - bit positions of 128-bit number
    ('./bcal', '-m', "1kib/ 4kb"),                                     # 58
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
    ('./bcal', '-m', "1 << 8 >> 4"),                                   # 76
    ('./bcal', '-m', "1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 1024"),  # 77
    ('./bcal', '-m', "1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1"),    # 78
    ('./bcal', '-b', "pow(2,128)"),                                    # 79
    ('./bcal', '-b', "1,2,3,4,5,6 + 1,000"),                           # 80
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
    b'\n\x1b[7m 31\x1b[0m \x1b[7m 30\x1b[0m \x1b[7m 29\x1b[0m \x1b[7m 28\x1b[0m \x1b[7m 27\x1b[0m \x1b[7m 26\x1b[0m \x1b[7m 25\x1b[0m \x1b[7m 24\x1b[0m \x1b[7m 23\x1b[0m \x1b[7m 22\x1b[0m \x1b[7m 21\x1b[0m \x1b[7m 20\x1b[0m \x1b[7m 19\x1b[0m \x1b[7m 18\x1b[0m \x1b[7m 17\x1b[0m \x1b[7m 16\x1b[0m \x1b[7m 15\x1b[0m \x1b[7m 14\x1b[0m \x1b[7m 13\x1b[0m \x1b[7m 12\x1b[0m \x1b[7m 11\x1b[0m \x1b[7m 10\x1b[0m \x1b[7m  9\x1b[0m \x1b[7m  8\x1b[0m \x1b[7m  7\x1b[0m \x1b[7m  6\x1b[0m \x1b[7m  5\x1b[0m \x1b[7m  4\x1b[0m \x1b[7m  3\x1b[0m \x1b[7m  2\x1b[0m \x1b[7m  1\x1b[0m \x1b[7m  0\x1b[0m \n  1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1 \n\n\x1b[7m 63\x1b[0m \x1b[7m 62\x1b[0m \x1b[7m 61\x1b[0m \x1b[7m 60\x1b[0m \x1b[7m 59\x1b[0m \x1b[7m 58\x1b[0m \x1b[7m 57\x1b[0m \x1b[7m 56\x1b[0m \x1b[7m 55\x1b[0m \x1b[7m 54\x1b[0m \x1b[7m 53\x1b[0m \x1b[7m 52\x1b[0m \x1b[7m 51\x1b[0m \x1b[7m 50\x1b[0m \x1b[7m 49\x1b[0m \x1b[7m 48\x1b[0m \x1b[7m 47\x1b[0m \x1b[7m 46\x1b[0m \x1b[7m 45\x1b[0m \x1b[7m 44\x1b[0m \x1b[7m 43\x1b[0m \x1b[7m 42\x1b[0m \x1b[7m 41\x1b[0m \x1b[7m 40\x1b[0m \x1b[7m 39\x1b[0m \x1b[7m 38\x1b[0m \x1b[7m 37\x1b[0m \x1b[7m 36\x1b[0m \x1b[7m 35\x1b[0m \x1b[7m 34\x1b[0m \x1b[7m 33\x1b[0m \x1b[7m 32\x1b[0m \n  1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1 \n\n\x1b[7m 95\x1b[0m \x1b[7m 94\x1b[0m \x1b[7m 93\x1b[0m \x1b[7m 92\x1b[0m \x1b[7m 91\x1b[0m \x1b[7m 90\x1b[0m \x1b[7m 89\x1b[0m \x1b[7m 88\x1b[0m \x1b[7m 87\x1b[0m \x1b[7m 86\x1b[0m \x1b[7m 85\x1b[0m \x1b[7m 84\x1b[0m \x1b[7m 83\x1b[0m \x1b[7m 82\x1b[0m \x1b[7m 81\x1b[0m \x1b[7m 80\x1b[0m \x1b[7m 79\x1b[0m \x1b[7m 78\x1b[0m \x1b[7m 77\x1b[0m \x1b[7m 76\x1b[0m \x1b[7m 75\x1b[0m \x1b[7m 74\x1b[0m \x1b[7m 73\x1b[0m \x1b[7m 72\x1b[0m \x1b[7m 71\x1b[0m \x1b[7m 70\x1b[0m \x1b[7m 69\x1b[0m \x1b[7m 68\x1b[0m \x1b[7m 67\x1b[0m \x1b[7m 66\x1b[0m \x1b[7m 65\x1b[0m \x1b[7m 64\x1b[0m \n  1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1 \n\n\x1b[7m127\x1b[0m \x1b[7m126\x1b[0m \x1b[7m125\x1b[0m \x1b[7m124\x1b[0m \x1b[7m123\x1b[0m \x1b[7m122\x1b[0m \x1b[7m121\x1b[0m \x1b[7m120\x1b[0m \x1b[7m119\x1b[0m \x1b[7m118\x1b[0m \x1b[7m117\x1b[0m \x1b[7m116\x1b[0m \x1b[7m115\x1b[0m \x1b[7m114\x1b[0m \x1b[7m113\x1b[0m \x1b[7m112\x1b[0m \x1b[7m111\x1b[0m \x1b[7m110\x1b[0m \x1b[7m109\x1b[0m \x1b[7m108\x1b[0m \x1b[7m107\x1b[0m \x1b[7m106\x1b[0m \x1b[7m105\x1b[0m \x1b[7m104\x1b[0m \x1b[7m103\x1b[0m \x1b[7m102\x1b[0m \x1b[7m101\x1b[0m \x1b[7m100\x1b[0m \x1b[7m 99\x1b[0m \x1b[7m 98\x1b[0m \x1b[7m 97\x1b[0m \x1b[7m 96\x1b[0m \n  1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1   1 \n\n\n',  # 57 - bit positions of 128-bit number
    b'WARNING: result truncated\n0\n',               # 58
    b'0\n',                                          # 59
    b'ERROR: first brackets only\n',                 # 60
    b'ERROR: first brackets only\n',                 # 61
    b'ERROR: invalid sequence >>>\n',                # 62
    b'ERROR: invalid operator <\n',                  # 63
    b'8388608\n',                                    # 64
    b'1219326312467611632.3609205901\n',             # 65
    b'36\n',                                         # 66
    b'36 B\n',                                       # 67
    b'1354\n',                                       # 68
    b'1069547520\n',                                 # 69
    b'WARNING: result truncated\n8391587\n',         # 70
    b'374 B\n',                                      # 71
    b'374\n',                                        # 72
    b'72000000\n',                                   # 73
    b'1\n',                                          # 74
    b'7\n',                                          # 75
    b'6\n',                                          # 76
    b'16\n',                                         # 77
    b'2047\n',                                       # 78
    b'1024\n',                                       # 79
    b'340282366920938463463374607431768211456\n',    # 80
    b'124456\n',                                     # 81
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

# REPL mode tests
def test_repl_basic_calculation():
    """Test basic byte calculation in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'10 mb\nq\n')
    assert b'10000000 B' in output
    assert b'bcal>' in output


def test_repl_show_last_result():
    """Test 'r' command to show last result in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'10 mb\nr\nq\n')
    assert b'10000000 B' in output
    assert b'r = 10000000 B' in output


def test_repl_toggle_expression_mode():
    """Test 'b' command to toggle general purpose expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n5 + 3\nq\n')
    assert b'general purpose expression mode' in output
    assert b'8' in output


def test_repl_show_sizes():
    """Test 's' command to show basic sizes in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b's\nq\n')
    # Output should contain size information
    assert b'bcal>' in output


def test_repl_help():
    """Test '?' command to show help in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'?\nq\n')
    assert b'bcal>' in output


def test_repl_quit_with_q():
    """Test 'q' command to quit REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'q\n')
    # Process should exit cleanly
    assert proc.returncode == 0


def test_repl_double_enter_to_quit():
    """Test double Enter to quit REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'\n\n')
    # Process should exit cleanly
    assert proc.returncode == 0


def test_repl_base_conversion_in_expr_mode():
    """Test 'c' command for base conversion in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc255\nq\n')
    assert b'general purpose expression mode' in output
    # Should show binary, decimal, and hex representations
    assert b'(b)' in output and b'(d)' in output and b'(h)' in output


def test_repl_bit_positions_in_expr_mode():
    """Test 'p' command for bit positions in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np255\nq\n')
    assert b'general purpose expression mode' in output


def test_repl_multiple_calculations():
    """Test multiple calculations in sequence in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'1 kb\n1 mb\nq\n')
    assert b'1000 B' in output
    assert b'1000000 B' in output


def test_repl_calculation_with_operators():
    """Test calculation with operators in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'10 mb * 2\nq\n')
    assert b'20000000 B' in output


def test_repl_invalid_input():
    """Test invalid input handling in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'invalid\nq\n')
    assert b'ERROR' in output or b'invalid' in output


def test_repl_empty_line_handling():
    """Test empty line handling in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'\n10 mb\nq\n')
    assert b'10000000 B' in output


def test_repl_whitespace_handling():
    """Test whitespace handling in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'  10   mb  \nq\n')
    assert b'10000000 B' in output


def test_repl_bitwise_operations():
    """Test bitwise operations in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'5 & 3\nq\n')
    assert b'1' in output


def test_repl_division_operation():
    """Test division operation in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'1 mib / 1 kib\nq\n')
    assert b'1024' in output


def test_repl_switching_between_modes():
    """Test switching between byte calc and expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'10 mb\nb\n5 + 3\nb\n10 kb\nq\n')
    assert b'10000000 B' in output
    assert b'general purpose expression mode' in output
    assert b'8' in output
    assert b'10000 B' in output


# Test existing cases in REPL mode
# Extract expressions and expected results for REPL testing
repl_test_cases = [
    # Byte calculations with -m flag
    ('10 mb', b'10000000 B\n'),                                              # 0
    ('10 TiB', b'10995116277760 B\n'),                                       # 1
    ('.1    KIb', b'102 B\n'),                                               # 2
    ('.1 KB', b'100 B\n'),                                                   # 3
    ('0x4         kb   *  2     +   5        mib', b'5250880 B\n'),          # 5
    ('5*5*5*5     mIB', b'655360000 B\n'),                                   # 6
    ('5mb*5*5*5', b'625000000 B\n'),                                         # 7
    ('2kb+3mb/4*5+5*56mb', b'283752000 B\n'),                                # 9
    ('( 5 * 3) * (4 * 7 b)', b'420 B\n'),                                    # 10
    ('( 5 * 3 + 8) * (4 * 7 b)', b'644 B\n'),                                # 11
    ('( 5 * (3 + 8 )) * (4 * 7 b )', b'1540 B\n'),                           # 12
    ('( 5 ) * 2 mib', b'10485760 B\n'),                                      # 13
    ('2giB*2/2', b'2147483648 B\n'),                                         # 18
    ('1miB / 4 kib', b'256\n'),                                              # 19
    ('(2giB*2)/2kib', b'2097152\n'),                                         # 20
    (' 1000 ', b'1000 B\n'),                                                 # 40
    ('0x18mb', b'24000000 B\n'),                                             # 43
    ('1kib/ 4kb', b'WARNING: result truncated\n0\n'),                        # 57 - no B unit in REPL
    ('0kib /4kb', b'0\n'),                                                   # 58
    ('2 >>> 2', b'ERROR: invalid sequence >>>\n'),                           # 61
    ('(2giB * 2) / (2kib >> 2)', b'8388608\n'),                              # 63
    ('4 + 3 - 2 + 10 * 5 / 2 + 7 - 6 + 9 - 10 / 5 * 2', b'36\n'),            # 65
    ('4b + 3b - 2b + 10b * 5 / 2 + 7b - 6b + 9b - 10b / 5 * 2', b'36 B\n'),  # 66
    ('(2.2giB * 2) / (2.2kib >> 2)', b'8391587\n'),                          # 69 - returns in base format in REPL
    ('0xbb b * 2', b'374 B\n'),                                              # 70
    ('5 & 3', b'1\n'),                                                       # 73
    ('5 | 2', b'7\n'),                                                       # 74
    ('5 ^ 3', b'6\n'),                                                       # 75
    ('1 << 8 >> 4', b'16\n'),                                                # 76
    ('1 | 2 | 4 | 8 | 16 | 32 | 64 | 128 | 256 | 512 | 1024', b'2047\n'),    # 77
    ('1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1 << 1', b'1024\n'),      # 78
]

# Error cases to verify error handling in REPL
repl_error_cases = [
    ('10 lb', b'ERROR: unknown unit\n'),                                     # 4
    ('2mb-3mib', b'ERROR: negative result\n'),                               # 15
    ('2 mib * -2', b'ERROR: negative token\n'),                              # 16
    ('((2giB)*(2/2)', b'ERROR: unbalanced expression\n'),                    # 24
    ('((2giB)*1)/(2/2))', b'ERROR: unbalanced expression\n'),                # 25
    ('(((2giB)*)2/2)', b'ERROR: invalid token\n'),                           # 27
    ('(2giB)*2*', b'ERROR: invalid token\n'),                                # 28
    ('2 / 3 tib', b'ERROR: unit mismatch in /\n'),                           # 39
]


@pytest.mark.parametrize('expr,expected', repl_test_cases)
def test_repl_existing_cases(expr, expected):
    """Test existing test cases in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=(expr + '\nq\n').encode())
    assert expected in output, f"Expected {expected} in output from REPL, got: {output}"


@pytest.mark.parametrize('expr,expected', repl_error_cases)
def test_repl_error_cases(expr, expected):
    """Test error cases in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=(expr + '\nq\n').encode())
    assert expected in output, f"Expected {expected} in output from REPL, got: {output}"


def test_repl_bit_positions_128bit():
    """Test bit positions of 128-bit number in REPL expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np0xffffffffffffffffffffffffffffffff\nq\n')
    assert b'general purpose expression mode' in output
    # Check for bit position numbers spanning all 4 32-bit groups (0-31, 32-63, 64-95, 96-127)
    assert b'127' in output and b'96' in output and b'63' in output and b'0' in output
    # All bits should be 1
    assert b'1   1   1   1   1   1   1   1' in output


def test_repl_unit_conversion():
    """Test unit conversion in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'1 gib\nq\n')
    assert b'1073741824 B' in output


def test_repl_c_base_conversion_decimal():
    """Test 'c' command for base conversion with decimal input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c255\nq\n')
    assert b'(b)' in output and b'11111111' in output
    assert b'(d) 255' in output
    assert b'(h) 0xff' in output


def test_repl_c_base_conversion_hex():
    """Test 'c' command for base conversion with hex input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c0xff\nq\n')
    assert b'(b)' in output and b'11111111' in output
    assert b'(d) 255' in output
    assert b'(h) 0xff' in output


def test_repl_c_base_conversion_binary():
    """Test 'c' command for base conversion with binary input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c0b11111111\nq\n')
    assert b'(b)' in output and b'11111111' in output
    assert b'(d) 255' in output
    assert b'(h) 0xff' in output


def test_repl_c_base_conversion_zero():
    """Test 'c' command for base conversion with zero in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c0\nq\n')
    assert b'(b) 0' in output
    assert b'(d) 0' in output
    assert b'(h) 0x0' in output


def test_repl_c_base_conversion_large_number():
    """Test 'c' command for base conversion with large number in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c0xffffffffffffffff\nq\n')
    assert b'(b)' in output and b'11111111' in output
    assert b'(d) 18446744073709551615' in output
    assert b'(h) 0xffffffffffffffff' in output


def test_repl_c_base_conversion_with_spaces():
    """Test 'c' command for base conversion with spaces in input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c  255  \nq\n')
    assert b'(b)' in output and b'11111111' in output
    assert b'(d) 255' in output
    assert b'(h) 0xff' in output


def test_repl_c_base_conversion_empty_input():
    """Test 'c' command for base conversion with empty input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c\nq\n')
    assert b'invalid input' in output


def test_repl_c_base_conversion_invalid_input():
    """Test 'c' command for base conversion with invalid input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c0x\nq\n')
    assert b'ERROR: invalid input' in output


def test_repl_p_bit_positions_simple():
    """Test 'p' command for bit positions with simple number in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p255\nq\n')
    # 255 = 0b11111111, so bits 0-7 should be set
    assert b'7' in output and b'6' in output and b'5' in output
    assert b'4' in output and b'3' in output and b'2' in output
    assert b'1' in output and b'0' in output


def test_repl_p_bit_positions_hex():
    """Test 'p' command for bit positions with hex input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p0xff\nq\n')
    # 0xff = 255 = 0b11111111, so bits 0-7 should be set
    assert b'7' in output and b'0' in output


def test_repl_p_bit_positions_binary():
    """Test 'p' command for bit positions with binary input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p0b11111111\nq\n')
    # 0b11111111 = 255, so bits 0-7 should be set
    assert b'7' in output and b'0' in output


def test_repl_p_bit_positions_power_of_two():
    """Test 'p' command for bit positions with power of 2 in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p1024\nq\n')
    # 1024 = 2^10 = 0b10000000000, so only bit 10 should be set
    assert b'10' in output


def test_repl_p_bit_positions_large_number():
    """Test 'p' command for bit positions with large 64-bit number in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p0xffffffffffffffff\nq\n')
    # All 64 bits should be set
    assert b'63' in output and b'32' in output and b'0' in output


def test_repl_p_bit_positions_128bit():
    """Test 'p' command for bit positions with 128-bit number in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p0xffffffffffffffffffffffffffffffff\nq\n')
    # All 128 bits should be set
    assert b'127' in output and b'96' in output and b'63' in output and b'0' in output


def test_repl_p_bit_positions_empty_input():
    """Test 'p' command for bit positions with empty input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p\nq\n')
    assert b'invalid input' in output


def test_repl_p_bit_positions_invalid_input():
    """Test 'p' command for bit positions with invalid input in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p0x\nq\n')
    assert b'ERROR: invalid input' in output


def test_repl_c_and_p_with_last_result():
    """Test using 'c' and 'p' commands after a calculation that stores result"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'10 mb\nc10000000\np10000000\nq\n')
    # First calculation: 10 mb = 10000000 B
    assert b'10000000 B' in output
    # 'c10000000' should convert 10000000 to different bases
    assert b'(d) 10000000' in output
    assert b'(h) 0x989680' in output
    # 'p10000000' should show bit positions for 10000000


def test_repl_c_in_storage_mode():
    """Test 'c' command works in default storage (byte calculation) mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c1024\nq\n')
    # Should convert without entering expression mode
    assert b'(d) 1024' in output
    assert b'(h) 0x400' in output


def test_repl_p_in_storage_mode():
    """Test 'p' command works in default storage (byte calculation) mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p1024\nq\n')
    # Should show bit positions without entering expression mode
    # 1024 = 2^10, so bit 10 should be set
    assert b'10' in output


def test_repl_c_and_p_in_expression_mode():
    """Test 'c' and 'p' commands in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc512\np512\nq\n')
    assert b'general purpose expression mode' in output
    # 'c512' should convert 512 to different bases
    assert b'(d) 512' in output
    # 'p512' should show bit positions for 512 (2^9)
    assert b'9' in output


def test_repl_c_multiple_conversions():
    """Test multiple 'c' commands in sequence in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c10\nc255\nc1024\nq\n')
    assert b'(d) 10' in output
    assert b'(d) 255' in output
    assert b'(d) 1024' in output


def test_repl_p_multiple_conversions():
    """Test multiple 'p' commands in sequence in REPL mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'p16\np128\np1024\nq\n')
    # 16 = 2^4, 128 = 2^7, 1024 = 2^10
    assert b'4' in output  # bit position for 16
    assert b'7' in output  # bit position for 128
    assert b'10' in output  # bit position for 1024


# Expression mode specific tests for 'c' and 'p' commands

def test_repl_c_expr_mode_decimal():
    """Test 'c' command with decimal input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc255\nq\n')
    assert b'general purpose expression mode' in output
    assert b'(b)' in output and b'11111111' in output
    assert b'(d) 255' in output
    assert b'(h) 0xff' in output


def test_repl_c_expr_mode_hex():
    """Test 'c' command with hex input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc0xabcd\nq\n')
    assert b'general purpose expression mode' in output
    assert b'(d) 43981' in output
    assert b'(h) 0xabcd' in output


def test_repl_c_expr_mode_binary():
    """Test 'c' command with binary input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc0b10101010\nq\n')
    assert b'general purpose expression mode' in output
    assert b'(d) 170' in output
    assert b'(h) 0xaa' in output


def test_repl_c_expr_mode_large_number():
    """Test 'c' command with large number in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc0xffffffffffffffff\nq\n')
    assert b'general purpose expression mode' in output
    assert b'(d) 18446744073709551615' in output
    assert b'(h) 0xffffffffffffffff' in output


def test_repl_c_expr_mode_with_expression_result():
    """Test 'c' command after expression calculation in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n16 * 16\nc256\nq\n')
    assert b'general purpose expression mode' in output
    assert b'256' in output
    # 'c256' should convert 256
    assert b'(d) 256' in output
    assert b'(h) 0x100' in output


def test_repl_c_expr_mode_empty_input():
    """Test 'c' command with empty input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc\nq\n')
    assert b'general purpose expression mode' in output
    assert b'invalid input' in output


def test_repl_c_expr_mode_invalid_input():
    """Test 'c' command with invalid input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc0x\nq\n')
    assert b'general purpose expression mode' in output
    assert b'ERROR: invalid input' in output


def test_repl_p_expr_mode_simple():
    """Test 'p' command with simple number in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np255\nq\n')
    assert b'general purpose expression mode' in output
    # 255 = 0b11111111, bits 0-7 should be set
    assert b'7' in output and b'0' in output


def test_repl_p_expr_mode_hex():
    """Test 'p' command with hex input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np0xff00\nq\n')
    assert b'general purpose expression mode' in output
    # 0xff00 has bits 8-15 set
    assert b'15' in output and b'8' in output


def test_repl_p_expr_mode_binary():
    """Test 'p' command with binary input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np0b11110000\nq\n')
    assert b'general purpose expression mode' in output
    # 0b11110000 = 240, bits 4-7 set
    assert b'7' in output and b'4' in output


def test_repl_p_expr_mode_power_of_two():
    """Test 'p' command with power of 2 in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np4096\nq\n')
    assert b'general purpose expression mode' in output
    # 4096 = 2^12
    assert b'12' in output


def test_repl_p_expr_mode_128bit():
    """Test 'p' command with 128-bit number in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np0xffffffffffffffffffffffffffffffff\nq\n')
    assert b'general purpose expression mode' in output
    # All 128 bits should be set
    assert b'127' in output and b'96' in output and b'63' in output and b'0' in output


def test_repl_p_expr_mode_with_expression_result():
    """Test 'p' command after shift expression calculation in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n1 << 8\np256\nq\n')
    assert b'general purpose expression mode' in output
    assert b'256' in output
    # 'p256' should show bit positions for 256 (bit 8 set)
    assert b'8' in output


def test_repl_p_expr_mode_empty_input():
    """Test 'p' command with empty input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np\nq\n')
    assert b'general purpose expression mode' in output
    assert b'invalid input' in output


def test_repl_p_expr_mode_invalid_input():
    """Test 'p' command with invalid input in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\np0b\nq\n')
    assert b'general purpose expression mode' in output
    assert b'ERROR: invalid input' in output


def test_repl_switch_modes_with_c_and_p():
    """Test switching between storage and expression modes using c and p"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'c100\nb\nc200\nb\nc300\nq\n')
    # Should work in both modes
    assert b'(d) 100' in output
    assert b'general purpose expression mode' in output
    assert b'(d) 200' in output
    assert b'(d) 300' in output


def test_repl_expr_mode_result_then_switch_to_storage():
    """Test using values after switching from expression to storage mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n50 * 5\nb\nc250\np250\nq\n')
    assert b'general purpose expression mode' in output
    assert b'250' in output
    # After switching back to storage mode, 'c250' and 'p250' should work
    assert b'(d) 250' in output
    assert b'(h) 0xfa' in output
    # 250 = 0xfa = 0b11111010, bits 1,3,4,5,6,7 set
    assert b'7' in output and b'1' in output


def test_repl_c_expr_mode_bitwise_calculation_result():
    """Test 'c' command with result from bitwise expression"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n15 & 7\ncr\nq\n')
    assert b'general purpose expression mode' in output
    assert b'7' in output  # 15 & 7 = 7
    assert b'(d) 7' in output
    assert b'(h) 0x7' in output


def test_repl_p_expr_mode_shift_calculation_result():
    """Test 'p' command with result from shift expression"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n1 << 10\npr\nq\n')
    assert b'general purpose expression mode' in output
    assert b'1024' in output
    # bit 10 should be set
    assert b'10' in output


def test_repl_c_and_p_multiple_in_expr_mode():
    """Test multiple 'c' and 'p' commands in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\nc128\np128\nc256\np256\nq\n')
    assert b'general purpose expression mode' in output
    assert b'(d) 128' in output
    assert b'(d) 256' in output
    # Check bit positions: 128 = 2^7, 256 = 2^8
    assert b'7' in output
    assert b'8' in output


def test_repl_expr_mode_arithmetic_then_convert():
    """Test arithmetic calculation followed by conversion in expression mode"""
    proc = subprocess.Popen('./bcal', stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    output, _ = proc.communicate(input=b'b\n(100 + 50) * 2\nc300\np300\nq\n')
    assert b'general purpose expression mode' in output
    assert b'300' in output
    assert b'(d) 300' in output
    # 300 = 0x12c = 0b100101100
    assert b'(h) 0x12c' in output
    # Bit positions for 300
