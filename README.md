<h1 align="center">bcal</h1>

<p align="center">
<a href="https://github.com/jarun/bcal/releases/latest"><img src="https://img.shields.io/github/release/jarun/bcal.svg?maxAge=600" alt="Latest release" /></a>
<a href="https://aur.archlinux.org/packages/bcal"><img src="https://img.shields.io/aur/version/bcal.svg?maxAge=600" alt="AUR" /></a>
<a href="https://packages.debian.org/search?keywords=bcal&searchon=names&exact=1"><img src="https://img.shields.io/badge/debian-9+-blue.svg?maxAge=2592000" alt="Debian Strech+" /></a>
<a href="http://packages.ubuntu.com/search?keywords=bcal&searchon=names&exact=1"><img src="https://img.shields.io/badge/ubuntu-17.04+-blue.svg?maxAge=2592000" alt="Ubuntu Zesty+" /></a>
<a href="https://github.com/jarun/bcal/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-yellow.svg?maxAge=2592000" alt="License" /></a>
</p>

<p align="center">
<a href="https://asciinema.org/a/96309"><img src="https://asciinema.org/a/96309.png" alt="bcal_asciicast" width="650"/></a>
</p>

`bcal` (*Byte CALculator*) is a command-line utility for storage conversions and calculations. Storage, hardware and firmware developers work with numerical calculations regularly e.g., storage unit conversions, address calculations etc. If you are one and can't calculate the hex address offset for (512 - 16) MiB immediately, or the value when the 43<sup>rd</sup> bit of a 64-bit address is set, `bcal` is for you.

Though it started with storage, the scope of `bcal` isn't limited to the storage domain. Feel free to raise PRs to simplify other domain-specific numerical calculations so it can evolve into an **engineer's utility**.

`bcal` follows Ubuntu's standard unit conversion and notation [policy](https://wiki.ubuntu.com/UnitsPolicy). Only 64-bit operating systems are supported.

<p align="center">
<a href="https://saythanks.io/to/jarun"><img src="https://img.shields.io/badge/say-thanks!-ff69b4.svg" /></a>
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RMLTQ76JSXJ4Q"><img src="https://img.shields.io/badge/PayPal-donate-FC746D.svg" alt="Donate via PayPal!" /></a>
</p>

### Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [Installing with a package manager](#installing-with-a-package-manager)
  - [Installing from this repository](#installing-from-this-repository)
    - [Get the source code](#get-the-source-code)
    - [Compile and install](#compile-and-install)
    - [Generated RPMs](#generated-rpms)
- [Usage](#usage)
  - [cmdline options](#cmdline-options)
  - [Operational notes](#operational-notes)
- [Examples](#examples)
- [Copyright](#copyright)

### Features

- evaluate arithmetic expressions involving storage units
- convert to IEC/SI standard data storage units
- show the address in bytes
- show address as LBA:OFFSET
- convert CHS to LBA and *vice versa*
- show binary, decimal and hex representation of a number
- custom sector size, max heads/cylinder and max sectors/track
- minimal dependencies

### Installation

#### Dependencies

`bcal` is written in C and depends on standard libc and [GCC libquadmath](https://gcc.gnu.org/onlinedocs/libquadmath/).

#### Installing with a package manager

`bcal` is also available on
 - [AUR](https://aur.archlinux.org/packages/bcal/)
 - [Debian Sid](https://packages.debian.org/sid/bcal)
 - [Ubuntu](http://packages.ubuntu.com/search?keywords=bcal&searchon=names&exact=1)
 - [Ubuntu PPA](https://launchpad.net/~twodopeshaggy/+archive/ubuntu/jarun/)

#### Installing from this repository

##### Get the source code

If you have git installed, clone this repository. Otherwise, download the latest [stable release](https://github.com/jarun/bcal/releases/latest) or [development version](https://github.com/jarun/bcal/archive/master.zip) (*risky*).

##### Compile and install

In the source directory, run:

    $ make
    $ sudo make install
To uninstall, run:

    $ sudo make uninstall

##### Generated RPMs

If you are on Fedora 24 or CentOS 7, visit [the latest stable release](https://github.com/jarun/bcal/releases/latest) and download the `.rpm` package for your OS.

### Usage

#### cmdline options

    usage: bcal [-c N] [-f FORMAT] [-s bytes] [-h]
                [expression] [N unit]

    Perform storage conversions and calculations.

    positional arguments:
      expression       evaluate storage arithmetic expression
                       +, -, *, / with decimal inputs supported
                       unit can be multipled or divided by +ve integer(s)
                       units can be added or subtracted from each other
                       Examples:
                           bcal "(5kb+2mb)/3"
                           bcal "5 tb / 12"
                           bcal "2.5mb*3"
      N unit           capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB
                       see https://wiki.ubuntu.com/UnitsPolicy
                       must be space-separated, case is ignored
                       N can be decimal or '0x' prefixed hex value

    optional arguments:
      -c N             show N in binary, decimal and hex
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
                       default MAX_HEAD: 16, default MAX_SECTOR: 63
      -s bytes         sector size [default 512]
      -h               show this help and exit

#### Operational notes

- **Expression**: An expression must be within double quotes. Inner spaces are ignored. A storage unit can only be divided or multiplied by plain integers. Only storage units can be used for addition and subtraction.
- **N unit**: `N` can be a decimal or '0x' prefixed hex value. `unit` can be B/KiB/MiB/GiB/TiB/kB/MB/GB/TB following Ubuntu policy. As all of these tokens are unique, `unit` is case-insensitive. `N` and `unit` must be space-separated.
- Only decimal is recognized in expressions. Decimal and hex **numeric representations** are recognized for unit conversions. Decimal, hex, binary and octal are recognized for all other operations.
- **Syntax**: Prefix hex inputs with `0x`, binary inputs with `0b`, octal inputs with `00`.
- **No negative arguments** allowed. Input limits are `unsigned long long` and `double`.
- **Fractional bytes do not exist**, because they can't be addressed. `bcal` shows the floor value of non-integer bytes.
- **CHS and LBA syntax**:
  - LBA: `lLBA-MAX_HEAD-MAX_SECTOR`   [NOTE: LBA starts with `l` (case ignored)]
  - CHS: `cC-H-S-MAX_HEAD-MAX_SECTOR` [NOTE: CHS starts with `c` (case ignored)]
  - Format conversion arguments must be hyphen separated.
  - Any unspecified value, including the one preceding the first `-` to the one following the last `-`, is considered `0` (zero).
- **Default values**:
  - sector size: 0x200 (512)
  - max heads per cylinder: 0x10 (16)
  - max sectors per track: 0x3f (63)

### Examples

1. Evaluate arithmetic expression of storage units.

        $ bcal "(5kb+2mb)/3"
        $ bcal "5 tb / 12"
        $ bcal "2.5mb*3"
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
7. Help and additional information.

        $ man bcal
        $ bcal -h

### Copyright

Copyright © 2016-2017 [Arun Prakash Jana](https://github.com/jarun)
