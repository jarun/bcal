<h1 align="center">bcal</h1>

<p align="center">
<a href="https://github.com/jarun/bcal/releases/latest"><img src="https://img.shields.io/github/release/jarun/bcal.svg" alt="Latest release" /></a>
<a href="https://aur.archlinux.org/packages/bcal"><img src="https://img.shields.io/aur/version/bcal.svg" alt="AUR" /></a>
<a href="https://github.com/jarun/bcal/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-yellow.svg?maxAge=2592000" alt="License" /></a>
</p>

<p align="center">
<a href="https://asciinema.org/a/87939"><img src="https://asciinema.org/a/87939.png" alt="bcal_asciicast" width="734"/></a>
</p>

`bcal` (*Byte CALculator*) is a command-line utility for storage conversions and calculations. Storage, hardware and firmware developers work with numerical calculations regularly e.g., storage unit conversions, address calculations etc. If you are one and can't calculate the hex address offset for (512 - 16) MiB immediately, or the value when the 43<sup>rd</sup> bit of a 64-bit address is set, `bcal` is for you.

Though it started with storage, the scope of `bcal` isn't limited to the storage domain. Feel free to raise PRs to simplify other domain-specific numerical calculations so it can evolve into an **engineer's tool**.

`bcal` follows Ubuntu's standard unit conversion and notation [policy](https://wiki.ubuntu.com/UnitsPolicy). Only 64-bit operating systems are supported.

<p align="center">
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RMLTQ76JSXJ4Q"><img src="https://img.shields.io/badge/paypal-donate-orange.svg?maxAge=2592000" alt="Donate" /></a>
</p>

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [Get the source code](#get-the-source-code)
  - [Compile and install](#compile-and-install)
  - [Installing with a package manager](#installing-with-a-package-manager)
  - [RPMs](#rpms)
- [Usage](#usage)
  - [cmdline options](#cmdline-options)
  - [Operational notes](#operational-notes)
- [Examples](#examples)
- [Copyright](#copyright)

## Features

- convert to IEC/SI standard data storage units
- show the address in bytes
- show address as LBA:OFFSET
- convert CHS to LBA and *vice versa*
- show binary, decimal and hex representation of a number
- custom sector size, max heads/cylinder and max sectors/track
- minimal dependencies

## Installation

### Dependencies

`bcal` is written in C and depends on standard libc and [GCC libquadmath](https://gcc.gnu.org/onlinedocs/libquadmath/).

### Get the source code

If you have git installed, run:

    $ git clone https://github.com/jarun/bcal
Otherwise, download the latest [stable release](https://github.com/jarun/bcal/releases/latest) or [development version](https://github.com/jarun/bcal/archive/master.zip).

### Compile and install

In the source directory, run:

    $ make
    $ sudo make install
To uninstall, run:

    $ sudo make uninstall

### Installing with a package manager

`bcal` is also available on
 - [AUR](https://aur.archlinux.org/packages/bcal/)
 - [Ubuntu PPA](https://launchpad.net/~twodopeshaggy/+archive/ubuntu/jarun/)

### RPMs

If you are on Fedora 24 or CentOS 7, visit [the latest stable release](https://github.com/jarun/bcal/releases/latest) and download the `.rpm` package for your OS.

## Usage

### cmdline options

    usage: bcal [-c N] [-s bytes] [-h]
            [N unit]

    Perform storage conversions and calculations.

    positional arguments:
      N unit           capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB
                       see https://wiki.ubuntu.com/UnitsPolicy
                       should be space-separated, case is ignored
                       N can be decimal or '0x' prefixed hex value

    optional arguments:
      -c N             show N in binary, decimal and hex
                       use prefix '0b' for binary, '0x' for hex
      -f FORMAT        convert CHS to LBA or LBA to CHS
                       formats are hyphen-separated
                       LBA format:
                           starts with 'l':
                           lLBA-MAX_HEAD-MAX_SECTOR
                       CHS format:
                           starts with 'c':
                           cC-H-S-MAX_HEAD-MAX_SECTOR
                       omitted values are considered 0
                       FORMAT 'c-50--0x12-' denotes:
                         C = 0, H = 50, S = 0, MH = 0x12, MS = 0
                       FORMAT 'l50-0x12' denotes:
                         LBA = 50, MH = 0x12, MS = 0
                       decimal or '0x' prefixed hex values accepted
                       default MAX_HEAD: 16, default MAX_SECTOR: 63
      -s bytes         sector size [decimal/hex, default 512]
      -h               show this help and exit

### Operational notes

- N unit: N can be decimal or '0x' prefixed hex value. Unit can be B/KiB/MiB/GiB/TiB/kB/MB/GB/TB following Ubuntu policy. As all of these tokens are unique, unit is case-insensitive.
- Fractional bytes do not exist, because they can't be addressed. `bcal` shows the floor value of non-integer bytes.
- Hex and decimal inputs are recognized for all inputs. Binary inputs are supported only with `-c`. This is a provision to convert a binary number to decimal/hex before using it as an input to other operations. No octal support.
- Syntax: Prefix hex inputs with `0x`, binary inputs with `0b`.
- Default values:
  - sector size: 0x200 (512)
  - max heads per cylinder: 0x10 (16)
  - max sectors per track: 0x3f (63)
- CHS and LBA inputs for format conversion are hyphen separated. The syntax is:
  - LBA: `lLBA-MAX_HEAD-MAX_SECTOR`   [NOTE: LBA starts with `l` (case ignored)]
  - CHS: `cC-H-S-MAX_HEAD-MAX_SECTOR` [NOTE: CHS starts with `c` (case ignored)]
  Any unspecified value, including the one preceding the first `-` to the one following the last `-`, is considered `0`.
- No negative numbers allowed. Input limits are `unsigned long long` and `double`.

## Examples

1. Convert storage capacity to other units and get address, LBA.

        $ bcal 20140115 b
        $ bcal 0x1335053 B
        $ bcal 0xaabbcc kb
        $ bcal 0xdef Gib
Note that the units are case-insensitive.

2. Convert storage capacity, set sector size to 4096 to calculate LBA.

        $ bcal 0xaabbcc kb -s 4096

3. Convert LBA to CHS.

        $ bcal -f l500
        $ bcal -f l0x600-18-0x7e
        $ bcal -f l0x300-0x12-0x7e

4. Convert CHS to LBA.

        $ bcal -f c10-10-10
        $ bcal -f c0x10-0x10-0x10
        $ bcal -f c0x10-10-2-0x12
        $ bcal -f c-10-2-0x12
        $ bcal -f c0x10-10--0x12

5. Show binary, decimal and hex representations of a number.

        $ bcal -c 20140115
        $ bcal -c 0b1001100110101000001010011
        $ bcal -c 0x1335053

6. Help and additional information.

        $ man bcal
        $ bcal -h

## Copyright

Copyright (C) 2016 [Arun Prakash Jana](mailto:engineerarun@gmail.com)
