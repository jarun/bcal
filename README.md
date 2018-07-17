<h1 align="center">bcal</h1>

<p align="center">
<a href="https://github.com/jarun/bcal/releases/latest"><img src="https://img.shields.io/github/release/jarun/bcal.svg?maxAge=600" alt="Latest release" /></a>
<a href="https://aur.archlinux.org/packages/bcal"><img src="https://img.shields.io/aur/version/bcal.svg?maxAge=600" alt="AUR" /></a>
<a href="http://formulae.brew.sh/formula/bcal"><img src="https://img.shields.io/homebrew/v/bcal.svg?maxAge=600" alt="Homebrew" /></a>
<a href="https://packages.debian.org/search?keywords=bcal&searchon=names&exact=1"><img src="https://img.shields.io/badge/debian-9+-blue.svg?maxAge=2592000" alt="Debian Stretch+" /></a>
<a href="https://apps.fedoraproject.org/packages/bcal"><img src="https://img.shields.io/badge/fedora-27+-blue.svg?maxAge=2592000" alt="Fedora 27+" /></a>
<a href="https://packages.ubuntu.com/search?keywords=bcal&searchon=names&exact=1"><img src="https://img.shields.io/badge/ubuntu-17.04+-blue.svg?maxAge=2592000" alt="Ubuntu Zesty+" /></a>
<a href="https://launchpad.net/~twodopeshaggy/+archive/ubuntu/jarun/"><img src="https://img.shields.io/badge/ubuntu-PPA-blue.svg?maxAge=2592000" alt="Ubuntu PPA" /></a>
<p>

<p align="center">
<a href="https://github.com/jarun/bcal/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-yellow.svg?maxAge=2592000" alt="License" /></a>
<a href="https://travis-ci.org/jarun/bcal"><img src="https://travis-ci.org/jarun/bcal.svg?branch=master" alt="Build Status" /></a>
</p>

<p align="center">
<a href="https://asciinema.org/a/168719"><img src="https://asciinema.org/a/168719.png" alt="bcal_asciicast" width="600"/></a>
</p>

`bcal` (*Byte CALculator*) is a command-line utility for storage, hardware and firmware developers who deal with storage-specific numerical calculations, expressions, unit conversions or address calculations frequently. If you are one and can't calculate the hex address offset for (512 - 16) MiB immediately, or the value when the 43<sup>rd</sup> bit of a 64-bit address is set, `bcal` is for you.

`bcal` follows Ubuntu's standard unit conversion and notation [policy](https://wiki.ubuntu.com/UnitsPolicy). Only 64-bit operating systems are supported.

*Love smart and efficient utilities? Explore [my repositories](https://github.com/jarun?tab=repositories). Buy me a cup of coffee if they help you.*

<p align="center">
<a href="https://saythanks.io/to/jarun"><img src="https://img.shields.io/badge/say-thanks!-ff69b4.svg" /></a>
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RMLTQ76JSXJ4Q"><img src="https://img.shields.io/badge/PayPal-donate-green.svg" alt="Donate via PayPal!" /></a>
</p>

### Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [From a package manager](#from-a-package-manager)
  - [Release packages](#release-packages)
  - [From source](#from-source)
- [Usage](#usage)
  - [cmdline options](#cmdline-options)
  - [Operational notes](#operational-notes)
- [Examples](#examples)
- [Testing](#testing)
- [Copyright](#copyright)

### Features

- evaluate arithmetic expressions involving storage units
- convert to IEC/SI standard data storage units
- interactive mode with the last valid result stored for reuse
- show the address in bytes
- show address as LBA:OFFSET
- convert CHS to LBA and *vice versa*
- base conversion to binary, decimal and hex
- custom sector size, max heads/cylinder and max sectors/track
- minimal dependencies

### Installation

#### Dependencies

`bcal` is written in C and depends on standard libc and libreadline. It tries GNU `bc` for expressions it cannot resolve.

#### From a package manager

- [AUR](https://aur.archlinux.org/packages/bcal/) (`pacman -S bcal`)
- [Debian](https://packages.debian.org/search?keywords=bcal&searchon=names&exact=1) (`apt-get install bcal`)
- [Fedora](https://apps.fedoraproject.org/packages/bcal) (`dnf install bcal`)
- [Homebrew](http://formulae.brew.sh/formula/bcal) (`brew install bcal`)
- [NixOS](https://github.com/NixOS/nixpkgs/tree/master/pkgs/applications/science/math/bcal) (`nix-env -i bcal`)
- [Ubuntu](https://packages.ubuntu.com/search?keywords=bcal&searchon=names&exact=1) (`apt-get install bcal`)
- [Ubuntu PPA](https://launchpad.net/~twodopeshaggy/+archive/ubuntu/jarun/) (`apt-get install bcal`)
- [Void Linux](https://github.com/voidlinux/void-packages/tree/master/srcpkgs/bcal) (`xbps-install -S bcal`)

#### Release packages

Packages for Arch Linux, CentOS, Debian, Fedora, OpenSUSE Leap and Ubuntu are available with the [latest stable release](https://github.com/jarun/bcal/releases/latest).

#### From source

If you have git installed, clone this repository. Otherwise, download the [latest stable release](https://github.com/jarun/bcal/releases/latest) or [development version](https://github.com/jarun/bcal/archive/master.zip) (*risky*).

Install to default location (`/usr/local`):

    $ make
    $ sudo make install
To uninstall, run:

    $ sudo make uninstall

`PREFIX` is supported, in case you want to install to a different location.

### Usage

#### cmdline options

```
usage: bcal [-c N] [-f FORMAT] [-s bytes] [-m] [-b] [-d]
            [expression] [N [unit]] [-h]

Storage expression calculator.

positional arguments:
 expression  evaluate storage arithmetic expression
             +, -, *, /, >>, << with decimal/hex operands
             Examples:
               bcal "(5kb+2mb)/3"
               bcal "5 tb / 12"
               bcal "2.5mb*3"
               bcal "(2giB * 2) / (2kib >> 2)"
 N [unit]    capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB
             see https://wiki.ubuntu.com/UnitsPolicy
             default unit is B (byte), case is ignored
             N can be decimal or '0x' prefixed hex value

optional arguments:
 -c N        show +ve integer N in binary, decimal, hex
 -f FORMAT   convert CHS to LBA or LBA to CHS
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
 -s bytes    sector size [default 512]
 -m          show minimal output (e.g. decimal bytes)
 -b          list sizes of basic data types in bytes
 -d          enable debug information and logs
 -h          show this help and exit
```

#### Operational notes

- **Interactive mode**: `bcal` enters the REPL mode if no arguments are provided. Storage unit conversion, base conversion and expression evaluation are supported in this mode. The last valid result is stored in the variable **r**.
- **Expression**: Expression passed as argument in one-shot mode must be within double quotes. Inner spaces are ignored.
- **N [unit]**: `N` can be a decimal or '0x' prefixed hex value. `unit` can be B/KiB/MiB/GiB/TiB/kB/MB/GB/TB following Ubuntu policy. Default is byte. As all of these tokens are unique, `unit` is case-insensitive.
- **Numeric representation**: Decimal and hex are recognized in expressions and unit conversions. Binary is also recognized in other operations.
- **Syntax**: Prefix hex inputs with `0x`, binary inputs with `0b`.
- **Precision**: 128 bits if `__uint128_t` is available or 64 bits for numerical conversions. Floating point operations use `long double`. Negative arguments are unsupported.
- **Fractional bytes do not exist** because they can't be addressed. `bcal` shows the floor value of non-integer _bytes_.
- **CHS and LBA syntax**:
  - LBA: `lLBA-MAX_HEAD-MAX_SECTOR`   [NOTE: LBA starts with `l` (case ignored)]
  - CHS: `cC-H-S-MAX_HEAD-MAX_SECTOR` [NOTE: CHS starts with `c` (case ignored)]
  - Format conversion arguments must be hyphen separated.
  - Any unspecified value, including the one preceding the first `-` to the one following the last `-`, is considered `0` (zero).
- **Default values**:
  - sector size: 0x200 (512)
  - max heads per cylinder: 0x10 (16)
  - max sectors per track: 0x3f (63)
- **bc variables**: scale = 5, ibase = 10. `bc` is not called in minimal output mode.

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
7. Help and additional information.

       $ man bcal
       $ bcal -h

### Testing

Due to the nature of the project, it's extremely important to test existing functionality before raising any PR. `bcal` has several test cases written in [`test.py`](test.py). To execute the test cases locally, install `pytest` and run:

    $ make
    $ python3 -m pytest test.py

### Copyright

Copyright © 2016-2018 [Arun Prakash Jana](https://github.com/jarun)
