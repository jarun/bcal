<h1 align="center">bcal</h1>

<p align="center">
<a href="https://github.com/jarun/bcal/releases/latest"><img src="https://img.shields.io/github/release/jarun/bcal.svg?maxAge=600" alt="Latest release" /></a>
<a href="https://repology.org/project/bcal/versions"><img src="https://repology.org/badge/tiny-repos/bcal.svg" alt="Availability"></a>
<a href="https://travis-ci.org/jarun/bcal"><img src="https://img.shields.io/travis/jarun/bcal/master.svg?label=travis" alt="Travis CI Status" /></a>
<a href="https://github.com/jarun/bcal/actions"><img src="https://github.com/jarun/bcal/workflows/ci/badge.svg?branch=master" alt="GitHub CI Status" /></a>
<a href="https://scan.coverity.com/projects/jarun-bcal"><img src="https://img.shields.io/coverity/scan/17148.svg" alt="Coverity Scan Build Status" /></a>
<a href="https://github.com/jarun/bcal/blob/master/LICENSE"><img src="https://img.shields.io/badge/license-GPLv3-yellowgreen.svg?maxAge=2592000" alt="License" /></a>
</p>

<p align="center">
<a href="https://asciinema.org/a/168719"><img src="https://asciinema.org/a/168719.svg" alt="bcal_asciicast" width="600"/></a>
</p>

`bcal` (*Byte CALculator*) is a REPL CLI utility for storage expression evaluation, unit conversion and address calculation. If you can't calculate the hex address offset for (512 - 16) MiB, or the value when the 43<sup>rd</sup> bit of a 64-bit address is set mentally, `bcal` is for you.

It has a [`bc`](https://www.gnu.org/software/bc/manual/html_mono/bc.html) mode for general-purpose numerical calculations. Alternatively, it can also invoke [`calc`](http://www.isthe.com/chongo/tech/comp/calc/) which works better with expressions involving multiple bases.

`bcal` follows Ubuntu's standard unit conversion and notation [policy](https://wiki.ubuntu.com/UnitsPolicy). Only 64-bit operating systems are supported.

*Love smart and efficient utilities? Explore [my repositories](https://github.com/jarun?tab=repositories). Buy me a cup of coffee if they help you.*

<p align="center">
<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=RMLTQ76JSXJ4Q"><img src="https://img.shields.io/badge/donate-@PayPal-1eb0fc.svg" alt="Donate via PayPal!" /></a>
</p>

### Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [From a package manager](#from-a-package-manager)
  - [Release packages](#release-packages)
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
- perform general purpose calculations (using bc or calc)
- works with piped input or file redirection
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

`bcal` is written in C and depends on standard libc and libreadline. It invokes GNU `bc` or `calc` for non-storage expressions.

To use `calc`:

    export BCAL_USE_CALC=1

#### From a package manager

Install `bcal` from your package manager. If the version available is dated try an alternative installation method.

<details><summary>Packaging status (expand)</summary>
<p>
<br>
<a href="https://repology.org/project/bcal/versions"><img src="https://repology.org/badge/vertical-allrepos/bcal.svg" alt="Packaging status"></a>
</p>
</details>

#### Release packages

Packages for Arch Linux, CentOS, Debian, Fedora, OpenSUSE Leap and Ubuntu are available with the [latest stable release](https://github.com/jarun/bcal/releases/latest).

#### From source

If you have git installed, clone this repository. Otherwise, download the [latest stable release](https://github.com/jarun/bcal/releases/latest) or [development version](https://github.com/jarun/bcal/archive/master.zip) (*risky*).

Install to default location (`/usr/local`):

    $ make
    $ sudo make strip install
To uninstall, run:

    $ sudo make uninstall

`PREFIX` is supported, in case you want to install to a different location.

#### Termux

`bcal` can be compiled and installed from source in the Termux environment on `aarch64` Android devices. Instructions:

```
$ aria2c https://github.com/jarun/bcal/archive/master.zip
$ unzip bcal-master.zip
$ cd bcal-master/
$ pkg install make clang readline-dev
$ make strip install
```

### Usage

#### cmdline options

```
usage: bcal [-c N] [-f loc] [-s bytes] [expr]
            [N [unit]] [-b [expr]] [-m] [-d] [-h]

Storage expression calculator.

positional arguments:
 expr       expression in decimal/hex operands
 N [unit]   capacity in B/KiB/MiB/GiB/TiB/kB/MB/GB/TB
            see https://wiki.ubuntu.com/UnitsPolicy
            default unit is B (byte), case is ignored
            N can be decimal or '0x' prefixed hex value

optional arguments:
 -c N       show +ve integer N in binary, decimal, hex
 -f loc     convert CHS to LBA or LBA to CHS
            refer to the operational notes in man page
 -s bytes   sector size [default 512]
 -b [expr]  enter bc mode or evaluate expression in bc
 -m         show minimal output (e.g. decimal bytes)
 -d         enable debug information and logs
 -h         show this help

prompt keys:
 b          toggle bc mode
 r          show result from last operation
 s          show sizes of storage types
 ?          show prompt help
 q/double ↵ quit program
```

#### Operational notes

- **Interactive mode**: `bcal` enters the REPL mode if no arguments are provided. Storage unit conversion, base conversion and expression evaluation are supported in this mode. The last valid result is stored in the variable **r**.
- **Expression**: Expression passed as argument in one-shot mode must be within double quotes. Inner spaces are ignored. Supported operators: `+`, `-`, `*`, `/`, `%` and C bitwise operators (except `~` due to storage width dependency).
- **N [unit]**: `N` can be a decimal or '0x' prefixed hex value. `unit` can be B/KiB/MiB/GiB/TiB/kB/MB/GB/TB following Ubuntu policy. Default is byte. As all of these tokens are unique, `unit` is case-insensitive.
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
- **bc variables**: `scale` = 10, `ibase` = 10. `r` is synced and can be used in expressions. `bc` is not called in minimal output mode.

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
7. Invoke `bc`.

       $ bcal -b '3.5 * 2.1 + 5.7'
       bcal> b  // Interactive mode
       bc vars: scale = 10, ibase = 10, last = r
       bc> 3.5 * 2.1 + 5.7
8. Pipe input.

       $ printf '15 kib + 15 gib \n r / 5' | bcal -m
       $ printf '15 + 15 + 2' | bcal -bm
9. Redirect from file.

       $ cat expr
       15 gib + 15 kib
       r / 5
       $ bcal -m < expr
10. Help and additional information.

        $ man bcal
        $ bcal -h

### Testing

Due to the nature of the project, it's extremely important to test existing functionality before raising any PR. `bcal` has several test cases written in [`test.py`](test.py). To execute the test cases locally, install `pytest` and run:

    $ make
    $ python3 -m pytest test.py

### Copyright

Copyright © 2016 [Arun Prakash Jana](https://github.com/jarun)
