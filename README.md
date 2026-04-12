# Retro ECMA-55-compliant Minimal BASIC Interpreter

**Endeavor: Retro-C**  
**Repository: \<[http://source.retro-c.net/comp.stdc.recma55](http://source.retro-c.net/comp.stdc.recma55)\>**  
**Version: 1.0.0**  
**Environments: C90, ASCII-CP**  
**Compliance: Retro-Frame 1.0**  
**License: MIT (see `LICENSE`)**  

Copyright (c) 2026 Ingo Boehmer \<ingo@retro-leisure.net\>

All product names, logos, and brands are property of their respective owners.


## Contents

1. Overview

2. How to build an executable from the source

3. Usage

4. Security

5. References


## 1. Overview

This repository contains the source code of an ECMA-55-compliant Minimal BASIC
interpreter (RECMA55) written in Standard C (C90). As this is about retro
programming, the benefit may be limited.

After build, the program can be run from the command line in order to execute
Minimal BASIC program files. A couple of ECMA-55-compliant **Minimal BASIC Test
Programs** and **Minimal BASIC Demo Programs** are provided by Retro-BASIC.

Development of this software is subject to Retro-C and complies to the
**Retro-Frame Common Documentation**.


## 2. How to build an executable from the source

The build of an executable from the source requires control files (e.g. build,
project and/or configuration files) depending on your development environment.

You may, of course, create your own control files and include the source file
from `src/` in order to build an executable. However, there are some control
files for selected development environments provided in `build/`.

### GCC / make

If you have the GNU Compiler Collection (GCC) and a make utility installed,
change to the directory `build/gcc/` and run the make utility (e.g. by `make`
or a similar command) in order to process the file `build/gcc/Makefile`.

The executable is created and stored in `build/gcc/bin`.

### Visual Studio

If you have Visual Studio installed, you may simply open one of the provided
solution files (`build/Visual Studio <version>/recma55.sln`). Each solution
file is implicitly linked to a Visual Studio project file (`recma55.vcxproj`).
Unfortunately, this file depends on two local prerequisites:

* Windows SDK and
* Platform toolset

While the platform toolset is related to the Visual Studio version, the Windows
SDK depends on the Windows version.

The project files provided match to specific Visual Studio versions (i.e. by
having the respective platform toolset versions configured). Choose the folder
of the Visual Studio version you are using.

If the Windows SDK version does not match, the build will fail with the
following error: "The Windows SDK version x.y was not found. [...]". In this
case, you may change the SDK version by right-clicking on the solution
respective project name (i.e. `recma55`) and then selecting "Retarget solution"
("SDK-Version neu ausrichten" in German). You may choose any target platform
version which is offered.


## 3. Usage

After build, run RECMA55 from the command line:

`RECMA55 [ <option> ]* [ <input-file> ]`

Arguments

* `<input-file>` ECMA-55 compliant Minimal BASIC program

Options

* `-SECURITY=LOW` sets the security level to low, which allows the usage of
  insecure statements and functions (see section 4).
* `-FULLCHAR` allows usage of the full native character set. Note that when a
  string is entered on an `INPUT` statement, it may be necessary to enclose the
  string in double-quotes.
* `-BATCH` enables batch processing (non-interactive mode), which suppresses
  the prompt on input and immediately fails on invalid input.
* `-NOBANNER` inhibits output of the banner at program start.
* `-LICENSE` outputs license text.

The options are not case sensitive.


## 4. Security

See the **Retro-Frame Common Documentation** guidelines for general security
aspects of retro programming and a specification of the security levels.

### Security level *medium*

By default, RECMA55 has security level *medium* (i.e. security has been
considered but not explicitly verified and the software has no known or
intentional flaws).

As a consequence, the function `RND` and the statement `RANDOMIZE` are not
available. If one of these appears during load of a BASIC program, the load
continues but the program is not executed (a security alert is printed
instead).

### Security level *low*

By command line option (see section 3), the security level can be set to *low*.

In this case, all functions and statements according to the ECMA-55 Minimal
BASIC standard are executed but you should be aware of the following potential
security risk:

A pseudo-random number generator (PRNG) is provided by the function `RND` and
the statement `RANDOMIZE`. This feature is implemented by the Standard C
functions `rand()` respective `srand()`, using the current time provided by the
function `time()` as seed or, if this function call fails, a seed provided by
user input.

These functions are not cryptographically secure. The user must not rely on the
randomness of the output and consider that the generated random numbers may
reveal the system time.


## 5. References

### ECMA-55 Minimal BASIC

ECMA-55 Minimal BASIC standard (withdrawn), European Computer Manufacturers
Association, January 1978, see
\<[https://ecma-international.org/publications-and-standards/standards/ecma-55/](https://ecma-international.org/publications-and-standards/standards/ecma-55/)\>.

### Minimal BASIC Demo Programs

ECMA-55 Minimal BASIC Demo Programs, Retro-BASIC, see
\<[http://source.retro-basic.net/ecma55.demo](http://source.retro-basic.net/ecma55.demo)\>.

### Minimal BASIC Test Programs

ECMA-55 Minimal BASIC Test Programs, Retro-BASIC, see
\<[http://source.retro-basic.net/ecma55.test](http://source.retro-basic.net/ecma55.test)\>.

### Retro-Frame Common Documentation

Retro-Frame Common Documentation, Retro-Frame, see
\<[http://source.retro-frame.net/common](http://source.retro-frame.net/common)\>.
