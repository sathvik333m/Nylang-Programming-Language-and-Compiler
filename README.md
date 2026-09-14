# Nylang Programming Language and Compiler

Nylang is a compact procedural programming language and a complete compiler implementation written in C. It takes Nylang source code from lexical analysis through parsing, semantic validation, and x86-64 NASM assembly generation, then can assemble, link, and run the resulting native program.

The project is designed as an approachable, end-to-end compiler: each phase can be inspected independently, while the `Nylang` command provides a straightforward compile-and-run workflow.

## Highlights

- Complete pipeline: Flex lexer -> Bison parser -> abstract syntax tree -> semantic analysis -> NASM x86-64 assembly
- Integers, strings, one-dimensional integer arrays, functions, and function calls
- Arithmetic, comparison, boolean, and unary expressions with precedence
- `if` / `elseif` / `else`, `while`, `for`, `break`, and `continue`
- Integer and full-line string input, plus `print` and `println`
- Scope-aware symbol management and semantic checks before code generation
- Compile-time and runtime array-bounds checks, division-by-zero checks, and input validation
- Inspection modes for tokens, ASTs, symbol tables, and semantic analysis

## Compiler Pipeline

```text
Nylang source (.ny)
        |
        v
   Flex lexer
        |
        v
  Bison parser + AST construction
        |
        v
 Semantic analysis and scoped symbols
        |
        v
 NASM x86-64 assembly
        |
        v
 Native executable
```

## Install Nylang

Nylang targets Linux on x86-64 and offers two installation routes:

1. Build from the GitHub source repository for development or to inspect the compiler.
2. Install the prebuilt Debian package with one command.

### 1. Build from GitHub Source

Install the following tools before building:

- GNU Make
- GCC
- Flex
- Bison
- NASM
- Bash

On Debian or Ubuntu:

```bash
sudo apt update
sudo apt install build-essential flex bison nasm
```

Clone the repository and build the compiler:

```bash
git clone https://github.com/sathvik333m/Nylang-Programming-Language-and-Compiler.git
cd Nylang-Programming-Language-and-Compiler
make compiler
```

### 2. Install the Prebuilt Compiler

For Debian or Ubuntu on x86-64, a prebuilt package is available. It installs the `nylang` command and compiler core without building from source:

```bash
wget -q https://github.com/sathvik333m/Nylang-Programming-Language-and-Compiler/releases/download/v1.0/nylang_1.0_amd64.deb -O nylang_1.0_amd64.deb && sudo apt install ./nylang_1.0_amd64.deb
```

Install `gcc` and `nasm` if needed, then compile a program:

```bash
sudo apt install gcc nasm
nylang hello.ny -o hello
./hello
```

See [installation_readme.md](installation_readme.md) for the package-installation guide.

## Run a Program

After building from source, run the included integrated demonstration:

```bash
./Nylang demo.ny
```

The demo requests a name and two integers. To provide them non-interactively:

```bash
printf 'Ada\n7\n3\n' | ./Nylang demo.ny
```

`./Nylang <source-file>` builds the compiler if needed, generates assembly, assembles and links it, then runs the native executable. The generated `.asm`, `.o`, and executable are placed in the project directory using the source file's base name.

## Your First Nylang Program

Create `hello.ny`:

```nylang
func add(left,right)
return left+right
endfunc

name="Nylang"
total=add(8,4)
println(name)
println(total)
```

Compile and run it:

```bash
./Nylang hello.ny
```

Expected output:

```text
Nylang
12
```

## Language Overview

| Area | Supported constructs |
| --- | --- |
| Functions | Top-level `func`, integer parameters, integer `return`, void-style functions |
| Values | Integers, string literals, string variables, and one-dimensional integer arrays |
| Expressions | `+`, `-`, `*`, `/`, `%`, comparisons, `and`, `or`, `not`, parentheses, unary minus |
| Flow control | `if` / `elseif` / `else`, `while`, `for`, `break`, `continue` |
| Input/output | `x=read()`, `read(name)`, `print(...)`, `println(...)` |
| Safety checks | Scope and type validation, function argument checks, array validation, runtime input/division/bounds checks |

### Arrays and Loops

```nylang
array values[5]

for(i=0; i<5; i=i+1)
values[i]=i*i
endfor

for(i=0; i<5; i=i+1)
println(values[i])
endfor
```

### Input and Conditions

```nylang
println("Enter an integer")
value=read()

if(value>10)
println("large")
else
println("small")
endif
```

## Compiler Modes

Use the compiler directly to examine individual stages:

| Command | Result |
| --- | --- |
| `./compiler --tokens program.ny` | Prints lexical tokens |
| `./compiler --ast program.ny` | Prints the abstract syntax tree |
| `./compiler --symbols program.ny` | Prints symbol-table information |
| `./compiler --semantic program.ny` | Runs semantic validation and reports the result |
| `./compiler program.ny > program.asm` | Emits NASM x86-64 assembly |
| `make phases` | Produces all inspection outputs for `program.ny` |

The `Nylang` wrapper accepts the four inspection flags too:

```bash
./Nylang --ast program.ny
```

## Project Structure

```text
.
|-- lexer.l                  # Flex token definitions
|-- parser.y                 # Bison grammar, compiler driver, and phase selection
|-- exprtree.c/.h            # AST representation and assembly code generation
|-- semantic.c/.h            # Static semantic analysis
|-- symboltable.c/.h         # Scope-aware symbol-table implementation
|-- Nylang                   # Build, assemble, link, and run wrapper
|-- Makefile                 # Compiler build and phase-output targets
|-- program.ny               # Integrated sample program
|-- demo.ny                  # Runnable demonstration program
|-- FEATURES.md              # Detailed feature and syntax reference
|-- NYLANG_PROJECT_REPORT.md # Technical design and implementation report
|-- installation_readme.md   # Prebuilt package installation guide
|-- nylang_1.0_amd64.deb     # Prebuilt Debian package
`-- README.md                # Setup, usage, and language overview
```

## Current Limitations

- No nested `return` inside `if`, `while`, or `for` blocks
- Empty control-flow bodies are not supported
- Arrays are one-dimensional integer arrays only
- Scalar variables are introduced through assignment; there are no explicit scalar declarations
- Functions do not support string parameters or string return values
- Local variables are not truly stack-allocated

## Documentation

- [Feature and syntax reference](FEATURES.md)
- [Technical project report](NYLANG_PROJECT_REPORT.md)
- [Debian/Ubuntu installation guide](installation_readme.md)

## License

No license has been specified for this repository. Add a license file before distributing or accepting external contributions under defined terms.
