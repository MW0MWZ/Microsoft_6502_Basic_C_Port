# Microsoft 6502 BASIC - C Port

A portable C implementation of Microsoft's original 6502 BASIC (1977), written in K&R C v2 for compatibility with older C compilers.

## Overview

This is a faithful port of Microsoft's original 6502 BASIC interpreter to portable C. The original 6502 BASIC was written by Bill Gates and Paul Allen and was licensed to many early microcomputer manufacturers including Apple, Commodore, and others.

## Features

### Statements
- `PRINT` - Output with TAB() and SPC() functions
- `INPUT` - User input with optional prompt
- `LET` - Variable assignment (implicit LET supported)
- `IF...THEN` - Conditional execution
- `FOR...NEXT` - Loop with optional STEP
- `GOTO` / `GOSUB` / `RETURN` - Control flow
- `ON...GOTO` / `ON...GOSUB` - Computed branching
- `DIM` - Array dimensioning
- `DATA` / `READ` / `RESTORE` - Data statements
- `DEF FN` - User-defined functions (basic support)
- `REM` - Comments
- `END` / `STOP` / `CONT` - Program control
- `NEW` / `LIST` / `RUN` - Program management
- `LOAD` / `SAVE` - File operations
- `POKE` / `PEEK` - Memory access (simulated)
- `CLEAR` - Clear variables
- `GET` - Single character input

### Functions

**Numeric:**
- `ABS`, `SGN`, `INT` - Basic math
- `SQR`, `LOG`, `EXP` - Powers and logarithms
- `SIN`, `COS`, `TAN`, `ATN` - Trigonometry
- `RND` - Random numbers with seeding
- `PEEK`, `FRE`, `POS` - System functions

**String:**
- `LEN`, `ASC`, `CHR$` - Character operations
- `LEFT$`, `RIGHT$`, `MID$` - Substrings
- `STR$`, `VAL` - String/number conversion

## Building

### macOS / Linux (GNU Make)

```bash
make
./m6502basic
```

### BSD Systems (BSD Make)

```bash
make -f Makefile.bsd
./m6502basic
```

### Clean Build

```bash
make clean
make
```

## Testing

Run the automated test suite:
```bash
printf "RUN\n" | ./m6502basic test/run_all_tests.bas
```

All 43 tests should pass with output ending in "ALL TESTS PASSED!"

Additional test programs are in the `test/` directory covering specific features.

## Usage

### Interactive Mode

```
$ ./m6502basic
MICROSOFT BASIC 6502
VERSION 1.1
(C) COPYRIGHT 1976-1978 MICROSOFT
C PORT (C) 2025 ANDY TAYLOR
65536 BYTES FREE

READY.
PRINT "HELLO WORLD"
HELLO WORLD
READY.
```

### Running Programs

```basic
10 REM HELLO WORLD
20 PRINT "HELLO, WORLD!"
30 END
RUN
```

### Loading and Saving

```basic
LOAD "program.bas"
RUN
SAVE "program.bas"
```

## Example Programs

The `examples/` directory contains sample programs:

- `hello.bas` - Hello World
- `forloop.bas` - FOR/NEXT loop test
- `math.bas` - Math functions test
- `strings.bas` - String functions test
- `arrays.bas` - Array operations
- `gosub.bas` - GOSUB/RETURN test
- `iftest.bas` - IF/THEN test
- `data.bas` - DATA/READ/RESTORE test
- `random.bas` - Random number test
- `nested.bas` - Nested loops (multiplication table)

## Technical Details

### K&R C Compatibility

This code is written in K&R C v2 style for compatibility with older C compilers:
- Old-style function definitions
- Variables declared at start of blocks
- No C99/C11 features
- 16-bit safe integer handling

### Memory Layout

Like the original 6502 BASIC:
- Program text stored as tokenized lines
- Variables in a linked list
- Arrays in a separate area
- String space grows downward from top of memory

### Tokens

Keywords are tokenized with MSB set (>= 128) for compact storage and fast parsing.

## Source Files

| File | Description |
|------|-------------|
| `m6502basic.h` | Main header with all definitions |
| `main.c` | Initialization and entry point |
| `repl.c` | Read-Eval-Print Loop, LOAD/SAVE |
| `tokenize.c` | Line tokenization |
| `eval.c` | Expression evaluator |
| `execute.c` | Statement execution engine |
| `statements.c` | Statement implementations |
| `functions.c` | Built-in functions |
| `variables.c` | Variable storage |
| `arrays.c` | Array handling |
| `strings.c` | String operations |
| `parse.c` | Program line management |
| `error.c` | Error handling |

## License

This C port is released under the MIT License. See LICENSE file.

The original Microsoft BASIC is Copyright (c) 1976-1978 Microsoft Corporation.

## Credits

- **Original 6502 BASIC**: Bill Gates, Paul Allen, Monte Davidoff - Microsoft (1977)
- **C Port**: Andy Taylor (2025)
