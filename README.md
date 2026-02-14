<h1 align="center">bcal</h1>

<p align="center">
<a href="https://github.com/jarun/bcal/releases/latest"><img src="https://img.shields.io/github/release/jarun/bcal.svg?maxAge=600&label=rel" alt="Latest release" /></a>
<a href="https://repology.org/project/bcal/versions"><img src="https://repology.org/badge/tiny-repos/bcal.svg?header=repos" alt="Availability"></a>
<a href="https://circleci.com/gh/jarun/workflows/bcal"><img src="https://img.shields.io/circleci/project/github/jarun/bcal.svg?label=CircleCI" alt="Circle CI Status" /></a>
<a href="https://github.com/jarun/bcal/actions"><img src="https://github.com/jarun/bcal/actions/workflows/ci.yml/badge.svg?branch=master" alt="GitHub CI Status" /></a>
<a href="https://scan.coverity.com/projects/jarun-bcal"><img src="https://img.shields.io/coverity/scan/17148.svg" alt="Coverity Scan Build Status" /></a>
<a href="https://github.com/jarun/bcal/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-yellowgreen.svg?maxAge=2592000" alt="License" /></a>
</p>

<p align="center">
<a href="https://asciinema.org/a/168719"><img src="https://asciinema.org/a/168719.svg" alt="bcal_asciicast" width="600"/></a>
</p>

`bcal` (*Byte CALculator*) is a REPL CLI utility for storage expression (e.g. `"(2GiB * 2) / (2KiB >> 2)"`) evaluation, SI/IEC conversion, byte address calculation, base conversion and LBA/CHS calculation. It's very useful for those who deal with bits, bytes, addresses and binary prefixes frequently.

It includes a built-in expression mode (`-b`) for general-purpose numerical calculations.

`bcal` uses [SI and IEC binary prefixes](https://en.wikipedia.org/wiki/Binary_prefix) and supports 64-bit Operating Systems only.

### Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [From a package manager](#from-a-package-manager)
  - [From source](#from-source)
  - [Termux](#termux)
- [Usage](#usage)
  - [cmdline options](#cmdline-options)
  - [Operational notes](#operational-notes)
- [Examples](#examples)
- [Testing](#testing)
- [Copyright](#copyright)

### Features

- REPL and single execution modes
- evaluate arithmetic expressions involving storage units
- perform general purpose calculations (built-in expression mode)
  - arithmetic operators: addition, subtraction, multiplication, division, modulo, exponentiation
  - bitwise operators: AND, OR, XOR, complement, left shift, right shift
  - functions: sqrt, cbrt, abs, floor, ceil, round, exp, log (base 10), ln (natural log), pow
- works with piped input or file redirection
- convert to IEC/SI standard data storage units
- interactive mode with the last valid result stored for reuse
- show the address in bytes
- show address as LBA:OFFSET
- convert CHS to LBA and *vice versa*
- base conversion to binary, decimal and hex
- show bit positions with bit value of a number
- custom sector size, max heads/cylinder and max sectors/track
- minimal dependencies

### Installation

#### Dependencies

`bcal` is written in C and depends on standard libc and GNU Readline (or [BSD Editline](https://www.thrysoee.dk/editline/)).

#### From a package manager

Install `bcal` from your package manager. If the version available is dated try an alternative installation method.

<details><summary>Packaging status (expand)</summary>
<p>
<br>
<a href="https://repology.org/project/bcal/versions"><img src="https://repology.org/badge/vertical-allrepos/bcal.svg" alt="Packaging status"></a>
</p>
</details>

#### From source

If you have git installed, clone this repository. Otherwise, download the [latest stable release](https://github.com/jarun/bcal/releases/latest) or [development version](https://github.com/jarun/bcal/archive/master.zip) (*risky*).

Install to default location (`/usr/local`):

    $ sudo make strip install
To link to libedit:

    $ sudo make O_EL=1 strip install
To uninstall, run:

    $ sudo make uninstall

`PREFIX` is supported, in case you want to install to a different location.

#### Termux

`bcal` can be compiled and installed from source in the Termux environment on `aarch64` Android devices. Instructions:

```
$ wget https://github.com/jarun/bcal/archive/master.zip
$ unzip bcal-master.zip
$ cd bcal-master/
$ pkg install make clang readline-dev
$ make strip install
```

### Usage

#### cmdline options

```
usage: bcal [-b [expr]] [-c N] [-p N] [-f loc] [-s bytes]
            [expr] [N [unit]] [-m] [-d] [-h]

Storage expression calculator.

positional arguments:
 expr       expression in decimal/hex operands
 N [unit]   capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB
            https://en.wikipedia.org/wiki/Binary_prefix
            default unit is B (byte), case is ignored
            N can be decimal or '0x' prefixed hex value

optional arguments:
 -b [expr]  enter expression mode or evaluate expression
 -c N       show +ve integer N in binary, decimal, hex
 -p N       show bit position (reversed if set) and value
 -f loc     convert CHS to LBA or LBA to CHS
            refer to the operational notes in man page
 -s bytes   sector size [default 512]
 -m         show minimal output (e.g. decimal bytes)
 -d         enable debug information and logs
 -h         show this help

prompt keys:
 b          toggle expression mode
 r          show result from last operation
 s          show sizes of storage types
 ?          show prompt help
 q/double ↵ quit program
```

#### Operational notes

- **Interactive mode**: `bcal` enters the REPL mode if no arguments are provided. Storage unit conversion, base conversion and expression evaluation are supported in this mode. The last valid result is stored in the variable **r**.
- **Expression**: Expression passed as argument in one-shot mode must be within double quotes. Inner spaces are ignored. Supported operators: `+`, `-`, `*`, `/`, `%` and C bitwise operators (except `~` due to storage width dependency).
- **N [unit]**: `N` can be a decimal or '0x' prefixed hex value. `unit` can be B/KiB/MiB/GiB/TiB/kB/MB/GB/TB. Default is Byte. As all of these tokens are unique, `unit` is case-insensitive.
- **Numeric representation**: Decimal and hex are recognized in expressions and unit conversions. Binary is also recognized in other operations.
- **Syntax**: Prefix hex inputs with `0x`, binary inputs with `0b`.
- **Precision**: 128 bits if `__uint128_t` is available or 64 bits for numerical conversions. Floating point operations use `long double`. Negative values in storage expressions are unsupported. Only 64-bit operating systems are supported.
- **Fractional bytes do not exist** because they can't be addressed. `bcal` shows the floor value of non-integer _bytes_.
- **CHS and LBA syntax**:
  - LBA: `lLBA-MAX_HEAD-MAX_SECTOR`   [NOTE: LBA starts with `l` (case ignored)]
  - CHS: `cC-H-S-MAX_HEAD-MAX_SECTOR` [NOTE: CHS starts with `c` (case ignored)]
  - Format conversion arguments must be hyphen separated.
  - Any unspecified value, including the one preceding the first `-` to the one following the last `-`, is considered `0` (zero).
  - Examples:
    - `c-50--0x12-` -> C = 0, H = 50, S = 0, MH = 0x12, MS = 0
    - `l50-0x12` -> LBA = 50, MH = 0x12, MS = 0
- **Default values**:
  - sector size: 0x200 (512)
  - max heads per cylinder: 0x10 (16)
  - max sectors per track: 0x3f (63)
- **Expression mode**: `r` is synced and can be used in expressions. The built-in evaluator uses `long double` arithmetic.

### Examples

1. Evaluate arithmetic expression of storage units.

       $ bcal "(5kb+2mb)/3"
       $ bcal "5 tb / 12"
       $ bcal "2.5mb*3"
       $ bcal "(2giB * 2) / (2kib >> 2)"
2. Convert storage capacity to other units and get address, LBA.

       $ bcal 20140115 b
       $ bcal 0x1335053 B
       $ bcal 0xaabbcc kb
       $ bcal 0xdef Gib
    Note that the units are case-insensitive.
3. Convert storage capacity, set sector size to 4096 to calculate LBA.

       $ bcal 0xaabbcc kb -s 4096
4. Convert LBA to CHS.

       $ bcal -f l500
       $ bcal -f l0x600-18-0x7e
       $ bcal -f l0x300-0x12-0x7e
5. Convert CHS to LBA.

       $ bcal -f c10-10-10
       $ bcal -f c0x10-0x10-0x10
       $ bcal -f c0x10-10-2-0x12
       $ bcal -f c-10-2-0x12
       $ bcal -f c0x10-10--0x12
6. Show binary, decimal and hex representations of a number.

       $ bcal -c 20140115
       $ bcal -c 0b1001100110101000001010011
       $ bcal -c 0x1335053
       bcal> c 20140115  // Interactive mode
7. Perform bitwise operations.

       $ bcal -b '0xFF & 0x0F'
       $ bcal -b '0x0F | 0xF0'
       $ bcal -b '0xFF ^ 0xF0'
       $ bcal -b '0xF0'
       $ bcal -b '0x01 << 3'
       $ bcal -b '0x10 >> 2'
       $ bcal -b '(0xFF & 0x0F) | (0x0F << 4)'
8. Use expression mode.

       $ bcal -b '3.5 * 2.1 + 5.7'
       bcal> b  // Interactive mode
       expr> 3.5 * 2.1 + 5.7
9. Pipe input.

       $ printf '15 kib + 15 gib \n r / 5' | bcal -m
       $ printf '15 + 15 + 2' | bcal -bm
10. Redirect from file.

       $ cat expr
       15 gib + 15 kib
       r / 5
       $ bcal -m < expr
11. Use as a general-purpose calculator.

        $ bcal -b
12. Show bit positions with values.

<img width="1033" height="138" alt="bcal bit position" src="https://github.com/user-attachments/assets/1b4a6c5e-8b3f-4d4b-a4dd-9045689f7dd8" />


### Testing

Due to the nature of the project, it's extremely important to test existing functionality before raising any PR. `bcal` has several test cases written in [`test.py`](test.py). To execute the test cases locally, install `pytest` and run:

    $ make
    $ python3 -m pytest test.py

### Copyright

Copyright © 2016 [Arun Prakash Jana](https://github.com/jarun)
