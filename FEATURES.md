# Nylang Compiler Features

This document summarizes the implemented Nylang language features, compiler phases, usage commands, and current limitations.

## 1. Functions

Nylang supports top-level function definitions.

Syntax:
```txt
func add(a,b)
return a+b
endfunc
```

Void-style function:
```txt
func greet()
println("hello")
endfunc
```

Function with body and return:
```txt
func compute(a,b)
x=a+b
return x
endfunc
```

Function call in expression:
```txt
x=add(10,20)
println(x)
```

Standalone function call statement:
```txt
greet()
```

Notes:
- Functions must be defined at top level.
- Parameters are integer parameters.
- A function without `return expr` is treated as void.
- A void function cannot be used inside an expression.
- Blank lines inside function bodies are supported.

## 2. Expressions

Nylang supports integer expressions with precedence and grouping.

Syntax:
```txt
x = a + b * c
y = (a + b) * c
z = -x
```

Supported operators:
- Arithmetic: `+`, `-`, `*`, `/`, `%`
- Comparison: `<`, `>`, `<=`, `>=`, `==`, `!=`
- Boolean: `and`, `or`, `not`
- Unary minus: `-expr`

Example:
```txt
t = (x > 10 and x < 20) or not (y == 0)
```

## 3. Control Flow

### `if`, `elseif`, `else`

Syntax:
```txt
if(x)
println("yes")
endif
```

```txt
if(x)
println("a")
elseif(y)
println("b")
else
println("c")
endif
```

### `while`

Syntax:
```txt
while(i<10)
println(i)
i=i+1
endwhile
```

### `for`

Syntax:
```txt
for(i=0; i<5; i=i+1)
println(i)
endfor
```

### `break` and `continue`

Syntax:
```txt
while(i<10)
if(i==5)
break
endif
i=i+1
endwhile
```

```txt
while(i<10)
i=i+1
continue
endwhile
```

Notes:
- Conditions are integer truth-style expressions.
- `break` and `continue` are valid only inside loops.
- Empty control-flow bodies like `if(x) endif` are not supported.

## 4. Arrays

One-dimensional integer arrays are supported.

Declaration syntax:
```txt
array nums[5]
```

Assignment syntax:
```txt
nums[0]=10
nums[i]=i+1
```

Access syntax:
```txt
println(nums[2])
println(nums[i])
```

Notes:
- Arrays are integer arrays only.
- Array bounds are checked:
  - compile-time for constant indices
  - runtime for variable/computed indices

## 5. Variables and Assignment

Scalar variables are supported for integers and strings.

Integer assignment:
```txt
x=10
y=x+1
```

String assignment:
```txt
msg="hello"
name=msg
```

Notes:
- Variables are introduced implicitly on first assignment or string read.
- Block scope is enforced semantically.

## 6. Input

### Integer input

Syntax:
```txt
x=read()
```

Behavior:
- Reads the next integer token from input.
- Accepts both:
```txt
4 2
```
and
```txt
4
2
```

### String input

Syntax:
```txt
read(msg)
```

Behavior:
- Reads one full line into a string variable.
- Supports spaces.
- Supports empty string input.

Examples:
```txt
read(name)
println(name)
```

## 7. Output

### `print`

Prints without newline.

Syntax:
```txt
print("A")
print(10)
```

### `println`

Prints with newline.

Syntax:
```txt
println("hello")
println(x)
```

Supported print values:
- integer expressions
- integer variables
- string literals
- string variables
- array element values
- function call results of int-returning functions

## 8. Strings

String literals and string variables are supported.

Literal syntax:
```txt
"hello"
"hello world"
""
```

Usage:
```txt
msg="hello world"
println(msg)
```

Notes:
- String input supports spaces and empty lines.
- String variables are stored in fixed-size buffers.

## 9. Scope Rules

Block scope is enforced semantically.

Example:
```txt
if(x)
temp=5
println(temp)
endif
```

Invalid outside the block:
```txt
println(temp)
```

Function-local variables are also handled separately from globals.

## 10. Identifier Rules

Identifiers support letters, underscores, and digits after the first character.

Valid examples:
```txt
x
value_2
my_var
count10
```

Pattern:
```txt
[a-zA-Z_][a-zA-Z0-9_]*
```

## 11. Semantic Analysis

The compiler performs semantic checks before code generation.

Checked items include:
- use of variables outside scope
- using arrays without indexing
- indexing non-arrays
- assignment type mismatches
- void function used in expressions
- wrong function argument count
- `break` / `continue` outside loops
- compile-time constant array out-of-bounds

Semantic phase command:
```bash
./compiler --semantic program.ny
```

## 12. Runtime Error Handling

Generated programs report runtime errors for:
- invalid integer input
- invalid string input
- division by zero
- array index out of bounds

Example messages:
```txt
Invalid integer input
Invalid string input
Division by zero
Array index out of bounds
```

## 13. AST Construction and AST Output

The parser builds an AST and can print it.

Command:
```bash
./compiler --ast program.ny
```

The AST includes nodes for:
- statements
- expressions
- function definitions
- function calls
- arrays
- loops
- conditionals
- input/output

## 14. Symbol Table Output

The compiler can print symbol-table information.

Command:
```bash
./compiler --symbols program.ny
```

Information includes:
- variable name
- inferred type
- binding
- size

## 15. Token Output

The lexer output can be inspected directly.

Command:
```bash
./compiler --tokens program.ny
```

This shows the token stream used by the parser.

## 16. Assembly Generation

The compiler generates NASM x86-64 assembly.

Command:
```bash
./compiler program.ny > program.asm
```

The output assembly includes:
- `.data` section
- `.text` section
- generated main code
- generated function code
- runtime helper routines
- runtime error handlers

## 17. Build and Phase Commands

### Build compiler
```bash
make compiler
```

### Generate all phase outputs
```bash
make phases
```

This produces:
- `tokens.out`
- `ast.out`
- `symbols.out`
- `semantic.out`
- `program.asm`

### Build and run a Nylang program
```bash
Nylang program.ny
```

If `Nylang` is not found, add the project folder to your `PATH` first:
```bash
export PATH="/home/sathvik/compiler_1:$PATH"
```

### Run automated verification
```bash
bash pre_submit_check.sh
```

This runs regression checks for:
- lexer output
- AST output
- symbol-table output
- semantic analysis
- code generation
- runtime behavior

## 18. Supported Program Layout

Nylang source files support:
- top-level function definitions
- top-level executable statements
- blank lines at top level
- blank lines inside function bodies
- any file name or extension; Nylang source files will typically use `.ny`

Typical structure:
```txt
func add(a,b)

x=a+b
return x

endfunc

println(add(2,3))
```

## 19. Current Known Limitations

These are the current known limitations:
- nested `return` statements inside `if` / `while` / `for`
- empty control-flow bodies such as:
```txt
if(x)
endif
```
- multi-dimensional arrays
- explicit scalar declarations
- true stack-allocated local variables
- string parameters / string return values for functions
