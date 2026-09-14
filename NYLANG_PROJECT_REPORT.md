%TITLE: Nylang Compiler Project Report
%SUBTITLE: Design, Implementation, Architecture, Language Features, and Demonstration
%DATE: 22 March 2026
%AUTHOR: Project Submission Copy

## Abstract

Nylang is a small compiler project that implements a custom programming language with integer expressions, string support, arrays, functions, control-flow constructs, semantic analysis, and x86-64 assembly generation. The project is designed as a complete compilation pipeline rather than an isolated parser or lexer exercise. Source programs written in Nylang are tokenized by Flex, parsed by Bison, converted into an abstract syntax tree, semantically validated using scoped symbol information, and finally translated into NASM-compatible assembly that can be assembled and linked into a native executable.

The most important outcome of the project is that it demonstrates the full path from language definition to running machine-level output. The compiler is able to inspect lexical tokens, print AST output, dump symbol-table information, perform semantic checks, and emit executable assembly. It also includes runtime support routines for integer input, string input, division-by-zero detection, and array bounds checking. This makes the project suitable not only as a language implementation assignment but also as a compact study in compiler organization.

This report presents the structure of the project, the architecture of the compiler, the syntax and semantics of the Nylang language, the testing strategy used during validation, and one integrated sample program that exercises the major implemented features. The report is intentionally written as a submission-ready technical document, so that a reviewer can understand both how the compiler is organized internally and what a user of the language can do with it.

## Table of Contents

- 1. Introduction and Objectives
- 2. Development Stack and Build Workflow
- 3. Project Structure
- 4. Compiler Architecture
- 5. Language Overview and Syntax Summary
- 6. Functions, Expressions, and Control Flow
- 7. Data Types, Arrays, Input, and Output
- 8. Semantic Analysis and Scope Handling
- 9. AST, Symbol Table, and Phase Outputs
- 10. Code Generation and Runtime Support
- 11. Testing and Verification
- 12. Large Integrated Nylang Sample Program
- 13. Sample Input and Sample Output
- 14. Limitations, Strengths, and Conclusion

[[PAGEBREAK]]

## 1. Introduction and Objectives

The purpose of the Nylang project is to build a small but meaningful programming language and implement the compiler components required to execute that language on a real machine. Instead of stopping at lexical analysis or parsing, the project follows the complete workflow expected in a simplified compiler:

- define a source language with clear syntax
- convert source text into tokens
- parse tokens into a tree representation
- validate program meaning using semantic rules
- maintain symbol information with scope awareness
- generate assembly code for the target machine
- provide runtime support for input, output, and safety checks

This end-to-end goal influenced the design of the language. Nylang is intentionally compact so that all stages remain understandable, but it is still expressive enough to show important compiler ideas. It supports top-level executable statements, top-level function definitions, integer and string values, arrays, conditional branching, loops, function calls, and both compile-time and runtime error handling.

From an academic perspective, the project demonstrates the interaction between static analysis and code generation. For example, some errors are rejected before assembly is generated, such as type mismatches, invalid argument counts, and illegal array usage. Other errors are intentionally handled at runtime, such as division by zero and dynamic array index failures. This distinction is a core compiler design idea, and the project exposes it clearly.

Another important objective was maintainability. The codebase is separated into clear modules for parsing, AST creation, semantic checking, and symbol-table management. This makes the project easier to explain, test, and extend. It also makes the report stronger, because the implementation is not just functional but also organized.

## 2. Development Stack and Build Workflow

The Nylang compiler is implemented in C and uses classic compiler-construction tools:

- Flex is used to generate the lexical analyzer from `lexer.l`.
- Bison is used to generate the parser from `parser.y`.
- GCC is used to compile the compiler itself and to link generated assembly into an executable program.
- NASM is used as the target assembler for x86-64 code generation.

The normal build workflow is controlled through the project `Makefile`. The main steps are:

- `make compiler`
  Builds the compiler binary from the Flex output, the Bison output, and the handwritten C modules.
- `make phases`
  Runs the compiler in token, AST, symbol-table, semantic, and assembly modes and writes the outputs to files.
- `Nylang program.ny`
  Compiles the default Nylang source program, assembles the generated code, links it, and executes it.

The project therefore supports two kinds of use:

- compiler development mode, where each phase is inspected separately
- language user mode, where a Nylang program is compiled and executed

The build flow is simple enough for demonstration but complete enough to reflect a real compiler pipeline. A reviewer can observe intermediate outputs and final execution using the same codebase.

## 3. Project Structure

The repository is compact and clearly divided into handwritten source files, generated files, and sample outputs. The following top-level structure captures the most important files in the project:

```txt
compiler_1/
  Makefile
  lexer.l
  parser.y
  exprtree.c
  exprtree.h
  semantic.c
  semantic.h
  symboltable.c
  symboltable.h
  pre_submit_check.sh
  FEATURES.md
  program.ny
  report_demo.ny
  report_demo_input.txt
  report_demo_output.txt
  lex.yy.c
  parser.tab.c
  parser.tab.h
  compiler
  tokens.out
  ast.out
  symbols.out
  semantic.out
  program.asm
  program.o
  program
```

The responsibilities of the major files are:

- `lexer.l`
  Defines the lexical rules for keywords, identifiers, numbers, strings, operators, separators, and whitespace behavior.
- `parser.y`
  Defines the grammar of Nylang, the driver program, parser actions, phase selection, and assembly emission helpers.
- `exprtree.c` and `exprtree.h`
  Define AST node creation, AST printing, memory cleanup, function emission, and the main code-generation logic.
- `semantic.c` and `semantic.h`
  Implement semantic validation such as type rules, scope checks, function-call checks, and compile-time array validation.
- `symboltable.c` and `symboltable.h`
  Implement symbol management, scope-aware lookups, data allocation metadata, and symbol-table dumping.
- `pre_submit_check.sh`
  Provides automated regression testing across the documented feature set.
- `FEATURES.md`
  Provides a concise user-facing summary of supported language features and project commands.

The generated artifacts are also meaningful:

- `lex.yy.c`, `parser.tab.c`, and `parser.tab.h` are generated from Flex and Bison.
- `compiler` is the built compiler executable.
- `tokens.out`, `ast.out`, `symbols.out`, and `semantic.out` are inspection outputs for the default program.
- `program.asm`, `program.o`, and `program` are the assembly, object, and executable produced from `program.ny`.

The codebase is not large by industrial standards, but it is substantial for a teaching compiler. The main implementation files contain more than four thousand lines combined across lexer, parser, AST, semantics, symbol-table logic, documentation, and testing support. A rough source-size snapshot is:

- `lexer.l`: 102 lines
- `parser.y`: 1011 lines
- `exprtree.c`: 1037 lines
- `semantic.c`: 716 lines
- `symboltable.c`: 408 lines
- `pre_submit_check.sh`: 369 lines

This structure supports both clarity and traceability. Each compiler phase has a clear home, and the project can be explained module by module.

## 4. Compiler Architecture

The internal architecture of Nylang follows a standard staged compiler pipeline:

```txt
Source File (.ny)
      |
      v
Lexical Analysis (Flex)
      |
      v
Parsing (Bison)
      |
      v
Abstract Syntax Tree Construction
      |
      v
Semantic Analysis + Scope Checking
      |
      v
Symbol-Aware Code Generation
      |
      v
NASM Assembly Output
      |
      v
Assembled and Linked Executable
```

Each stage contributes different responsibilities.

Lexical analysis transforms plain text into a token stream. This stage recognizes language keywords such as `func`, `if`, `while`, `for`, `read`, `print`, and `println`, along with identifiers, numeric literals, string literals, arithmetic operators, logical operators, and separators such as newline and semicolon.

Parsing enforces the grammatical structure of the language. The parser validates that expressions and statements appear in the proper order and uses grammar actions to create AST nodes. Nylang supports top-level functions as well as top-level executable statements, so the parser separates the root program into a function tree and a main-program tree.

Semantic analysis then verifies meaning rather than syntax. For example, a program may be grammatically valid but still incorrect if it uses a variable outside scope, applies array indexing to a scalar variable, or calls a function with the wrong number of arguments. This stage prevents invalid assembly from being generated.

Finally, code generation converts the validated AST into x86-64 NASM assembly. This stage is where expressions are evaluated, loops are lowered into labels and jumps, arrays are accessed with indexed addressing, functions are emitted as callable labels, and runtime helper routines are attached.

The architecture is effective because each stage narrows the problem. The lexer answers "what symbols exist", the parser answers "how are they structured", the semantic analyzer answers "is the program meaningful", and the code generator answers "how can this run on hardware".

## 5. Language Overview and Syntax Summary

Nylang is designed as a procedural language with lightweight syntax and a newline-driven style. Source files typically contain function definitions first, followed by top-level executable statements, although both are permitted at top level according to the parser organization.

The language currently supports the following high-level feature categories:

- top-level integer functions
- top-level executable statements
- integer expressions
- string literals and string variables
- one-dimensional integer arrays
- `if`, `elseif`, `else`
- `while` loops
- `for` loops
- `break` and `continue`
- integer input and string input
- `print` and `println`
- semantic checks before assembly generation
- assembly emission for x86-64

Identifiers follow the pattern:

```txt
[a-zA-Z_][a-zA-Z0-9_]*
```

This means names can begin with a letter or underscore and can contain digits after the first character. Examples include:

- `x`
- `count10`
- `value_2`
- `my_var`

Expressions are integer-based and support precedence. Representative syntax looks like:

```txt
x=a+b*c
y=(a+b)*c
z=-x
flag=(x>10 and x<20) or not(y==0)
```

The grammar also supports semicolon-separated control clauses in `for` loops:

```txt
for(i=0; i<5; i=i+1)
println(i)
endfor
```

String literals are enclosed in double quotes:

```txt
"hello"
"Nylang"
""
```

Arrays are declared using a dedicated keyword:

```txt
array nums[5]
nums[0]=10
println(nums[0])
```

Because Nylang is intentionally small, syntax remains readable, and most constructs can be learned quickly from examples.

## 6. Functions, Expressions, and Control Flow

Functions are one of the strongest features of the language. Nylang supports integer-valued functions and void-style procedures. A function is defined at top level using `func` and terminated with `endfunc`. Integer parameters are supported, and a final `return expr` may be used for functions that produce a value.

Typical function syntax:

```txt
func add(a,b)
result=a+b
return result
endfunc
```

Void-style function syntax:

```txt
func greet()
println("hello")
endfunc
```

Function calls can appear either as standalone statements or inside expressions:

```txt
greet()
x=add(10,20)
println(add(2,3))
```

Expression support includes:

- arithmetic operators: `+`, `-`, `*`, `/`, `%`
- comparisons: `<`, `>`, `<=`, `>=`, `==`, `!=`
- boolean operators: `and`, `or`, `not`
- unary minus
- parentheses for grouping

Control flow includes conditional branching and looping. Nylang supports:

- `if`
- `if` / `elseif` / `else`
- `while`
- `for`
- `break`
- `continue`

Example conditional:

```txt
if(x>0)
println("positive")
elseif(x==0)
println("zero")
else
println("negative")
endif
```

Example loop:

```txt
while(i<10)
println(i)
i=i+1
endwhile
```

The language treats conditions as integer truth expressions, so zero is false and non-zero is true. This makes expression evaluation and branching simpler during code generation.

## 7. Data Types, Arrays, Input, and Output

Nylang primarily works with three value categories at compile time:

- integers
- strings
- integer arrays

Scalar variables do not need explicit declarations. They are introduced on first assignment or, in the case of strings, on first string read:

```txt
x=10
msg="hello"
read(name)
```

This choice keeps the language lightweight, but it shifts more responsibility to semantic analysis. The compiler must infer and preserve the type associated with each identifier.

Arrays are fixed-size, one-dimensional, and integer-only. Example:

```txt
array nums[5]
nums[0]=10
nums[1]=20
println(nums[1])
```

Input is intentionally split into two forms:

- `x=read()`
  reads an integer expression value from standard input
- `read(name)`
  reads a full input line into a string variable

This design avoids ambiguity between numeric and string input and allows the semantic analyzer to reject invalid uses such as assigning string input to an integer variable.

Output is also intentionally simple:

- `print(expr)` prints without a newline
- `println(expr)` prints with a newline

Supported output operands include:

- integer expressions
- integer variables
- string literals
- string variables
- array elements
- integer-returning function calls

The runtime support code generated by the compiler includes helper routines for integer parsing, line-based string reading, and error reporting. This means the source language remains compact while the generated program still behaves like a usable command-line program.

## 8. Semantic Analysis and Scope Handling

Semantic analysis is a major strength of the project because it moves the compiler beyond syntax checking into meaning-aware validation. The semantic analyzer verifies:

- variable use before declaration or outside visible scope
- invalid array use without indexing
- invalid indexing of non-array variables
- assignment type mismatches
- use of void functions in expressions
- incorrect function argument counts
- `break` and `continue` outside loops
- compile-time constant array out-of-bounds access

Scope is handled with an explicit semantic scope stack. Blocks introduced by `if`, `else`, `while`, and `for` create nested semantic environments. A variable introduced inside a block is not visible after the block ends. This allows the compiler to reject programs such as:

```txt
if(x)
temp=5
endif
println(temp)
```

Function-local variables are handled separately from globals. Internally, they are name-mangled so that generated assembly avoids collisions between function locals and top-level variables. This is a pragmatic solution that preserves the language abstraction while keeping the code generator manageable.

Semantic analysis also distinguishes between compile-time and runtime safety. If an array index is a constant and provably out of bounds, the compiler reports an error before code generation. If the index is computed dynamically, the generated program performs a runtime check. This design is both safe and pedagogically valuable because it illustrates different categories of error handling.

## 9. AST, Symbol Table, and Phase Outputs

The Nylang compiler provides multiple inspection modes, which is an excellent feature for debugging, demonstration, and grading. These modes are selected using command-line flags:

- `--tokens`
- `--ast`
- `--symbols`
- `--semantic`

The token phase prints the lexical stream recognized by the lexer. This is useful when validating keywords, identifiers, operators, and separators.

The AST phase prints the abstract syntax tree in a readable hierarchical structure. This allows the user to verify parsing and operator nesting. It is especially useful when checking whether control-flow statements and function calls are being represented correctly.

The symbol-table phase prints discovered identifiers together with type, binding, size, and scope metadata. This gives a concise view of how the compiler understands names in the program.

The semantic phase either reports success or prints semantic error messages. By separating this from assembly generation, the project makes it easy to see whether a failure is syntactic, semantic, or code-generation related.

The symbol table itself stores information such as:

- variable or array name
- type
- memory binding
- size
- scope path
- scope depth

The AST is represented through a single node structure with different `type`, `op`, and child-pointer combinations. This compact representation supports expressions, statements, functions, arrays, reads, writes, and control-flow nodes using one shared tree model. That design reduces the amount of boilerplate required in the rest of the compiler.

## 10. Code Generation and Runtime Support

After a program passes semantic analysis, the compiler emits NASM x86-64 assembly. The assembly generator is implemented in `exprtree.c`, while `parser.y` emits the surrounding runtime helper routines and section-level scaffolding. The generated assembly contains:

- a `.data` section for variables, arrays, string storage, format strings, and error messages
- a `.text` section for functions and the main entry point
- helper routines for line reading and integer parsing
- error labels for division by zero, invalid input, and array bounds failures

The code generator maps AST operations into stack-based evaluation patterns. For arithmetic expressions, child expressions are generated first, their values are pushed to the stack, and the final machine operation consumes those values. Comparisons similarly generate integer truth values. Loop constructs become labels and conditional jumps. Functions become named labels prefixed for separation from runtime helpers.

Runtime support includes several important safety behaviors:

- integer input validation
- string input validation
- array bounds checking for dynamic indices
- division-by-zero detection

This is an important design decision. The compiler is not just producing assembly mechanically; it is generating protected execution paths that enforce the rules promised by the language design.

The generated code is intended for educational clarity rather than aggressive optimization. That is appropriate for the project goals. The output is readable, systematic, and directly traceable to the source-level constructs supported by the language.

## 11. Testing and Verification

The project includes both manual phase inspection and automated regression testing. This is a major submission advantage because it demonstrates confidence in correctness rather than relying only on example runs.

The script `pre_submit_check.sh` runs a set of regression tests across the implemented feature set. These tests cover:

- functions and local-variable handling
- expressions and identifier rules
- `while` with `break` and `continue`
- `for` loops
- arrays
- strings
- integer input
- string input
- signed division and modulo behavior
- semantic success and semantic failure cases
- runtime error paths
- token output
- AST output
- symbol-table output
- assembly generation
- parsing limitations that are intentionally documented

The verification pass used for the final project state reported:

- 27 checks passed
- 0 checks failed

This does not mean the compiler is perfect, but it does mean that the documented and implemented feature surface was actively validated. For a compiler assignment, that is a strong sign of submission readiness.

## 12. Large Integrated Nylang Sample Program

The following sample program was prepared specifically for this report. It exercises the major implemented features together in one coherent example:

- multiple top-level functions
- integer-returning functions
- void-style function call statements
- arithmetic expressions
- boolean logic
- unary minus
- arrays
- `for` loops
- `while`-style behavior through loop control logic
- `if`, `elseif`, `else`
- `break`
- `continue`
- string input
- integer input
- `print` and `println`
- string variables and string literals

```txt
{{INCLUDE:report_demo.ny}}
```

This example is intentionally larger than the minimal examples used in feature summaries. It is designed to demonstrate how Nylang can coordinate input, computation, arrays, function calls, classification logic, and formatted output inside a single program.

## 13. Sample Input

The integrated demo above was executed using the following input:

```txt
{{INCLUDE:report_demo_input.txt}}
```

The first line is consumed by `read(username)` as a string input. The second line provides two integers that are read by consecutive `read()` expressions.

## 14. Sample Output

The exact output produced by the integrated demo program is shown below:

```txt
{{INCLUDE:report_demo_output.txt}}
```

This output confirms several aspects of the compiler and runtime:

- function calls returned correct integer values
- string input was captured correctly
- arithmetic and logical expressions produced the expected results
- arrays were initialized and read correctly
- `continue` skipped one iteration in the filtering loop
- `break` terminated the loop after a threshold was reached
- conditional classification logic selected the correct branch
- formatted program output remained readable from start to finish

## 15. Limitations, Strengths, and Conclusion

Like any compact compiler project, Nylang has clear limits. The most important known limitations are:

- nested `return` statements inside `if`, `while`, and `for` blocks are not supported
- empty control-flow bodies are not supported
- arrays are one-dimensional only
- scalar declarations are implicit rather than explicit
- true stack-allocated local variables are not implemented
- string parameters and string return values for functions are not implemented

These limitations are reasonable for the project scope. In fact, they help preserve the clarity of the compiler pipeline and keep the implementation explainable. The project does not attempt to solve every language-design problem; instead, it focuses on a coherent subset and implements that subset fully.

The strongest aspects of the project are:

- complete end-to-end compilation from source program to native executable
- well-separated modules for lexer, parser, AST, semantics, and symbols
- readable phase outputs for debugging and explanation
- practical runtime safety checks
- automated regression testing
- a custom language identity with `.ny` source files and the name Nylang

In conclusion, Nylang is a strong compiler project because it combines language design, static analysis, runtime support, and assembly generation in one integrated system. It is small enough to understand, but large enough to demonstrate genuine compiler engineering. The project is therefore suitable both as a course submission and as a foundation for future extensions such as richer type systems, multidimensional arrays, improved function semantics, or more advanced backend generation.
