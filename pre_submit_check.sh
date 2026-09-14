#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
TMP_DIR="$(mktemp -d /tmp/nylang-check.XXXXXX)"
PASS_COUNT=0
FAIL_COUNT=0

cleanup() {
    rm -rf "$TMP_DIR"
}

trap cleanup EXIT

pass() {
    printf 'PASS %s\n' "$1"
    PASS_COUNT=$((PASS_COUNT + 1))
}

fail() {
    printf 'FAIL %s: %s\n' "$1" "$2"
    FAIL_COUNT=$((FAIL_COUNT + 1))
}

make -C "$ROOT_DIR" compiler >/dev/null

write_case() {
    local name="$1"
    local source="$2"
    local path="$TMP_DIR/$name.ny"

    printf '%s' "$source" > "$path"
    printf '%s\n' "$path"
}

build_binary() {
    local src="$1"
    local stem="$2"
    local asm="$TMP_DIR/$stem.asm"
    local obj="$TMP_DIR/$stem.o"
    local bin="$TMP_DIR/$stem.bin"

    "$ROOT_DIR/compiler" "$src" > "$asm"
    nasm -felf64 "$asm" -o "$obj"
    gcc "$obj" -no-pie -o "$bin" >/dev/null 2>&1
    printf '%s\n' "$bin"
}

expect_run() {
    local name="$1"
    local source="$2"
    local expected_output="$3"
    local input_data="${4-}"
    local src
    local bin
    local input_file="$TMP_DIR/$name.in"
    local output_file="$TMP_DIR/$name.out"
    local actual_output

    src="$(write_case "$name" "$source")"

    if ! bin="$(build_binary "$src" "$name" 2>"$TMP_DIR/$name.build.err")"; then
        fail "$name" "build failed: $(tr '\n' ' ' < "$TMP_DIR/$name.build.err")"
        return
    fi

    printf '%s' "$input_data" > "$input_file"
    if ! "$bin" < "$input_file" > "$output_file" 2>&1; then
        fail "$name" "program exited with failure: $(tr '\n' ' ' < "$output_file")"
        return
    fi

    actual_output="$(cat "$output_file")"
    if [[ "$actual_output" != "$expected_output" ]]; then
        fail "$name" "expected [$expected_output] but got [$actual_output]"
        return
    fi

    pass "$name"
}

expect_runtime_error() {
    local name="$1"
    local source="$2"
    local expected_text="$3"
    local input_data="${4-}"
    local src
    local bin
    local input_file="$TMP_DIR/$name.in"
    local output_file="$TMP_DIR/$name.out"
    local status

    src="$(write_case "$name" "$source")"

    if ! bin="$(build_binary "$src" "$name" 2>"$TMP_DIR/$name.build.err")"; then
        fail "$name" "build failed: $(tr '\n' ' ' < "$TMP_DIR/$name.build.err")"
        return
    fi

    printf '%s' "$input_data" > "$input_file"
    set +e
    "$bin" < "$input_file" > "$output_file" 2>&1
    status=$?
    set -e

    if [[ $status -eq 0 ]]; then
        fail "$name" "expected runtime failure but program succeeded"
        return
    fi

    if ! grep -Fq "$expected_text" "$output_file"; then
        fail "$name" "expected runtime message [$expected_text], got [$(tr '\n' ' ' < "$output_file")]"
        return
    fi

    if [[ $status -ne 1 ]]; then
        fail "$name" "expected exit code 1 but got $status"
        return
    fi

    pass "$name"
}

expect_semantic_success() {
    local name="$1"
    local source="$2"
    local src
    local output_file="$TMP_DIR/$name.semantic.out"

    src="$(write_case "$name" "$source")"
    if ! "$ROOT_DIR/compiler" --semantic "$src" > "$output_file" 2>&1; then
        fail "$name" "semantic analysis unexpectedly failed: $(tr '\n' ' ' < "$output_file")"
        return
    fi

    if ! grep -Fq "Semantic analysis: success" "$output_file"; then
        fail "$name" "semantic success banner missing"
        return
    fi

    pass "$name"
}

expect_semantic_error() {
    local name="$1"
    local source="$2"
    local expected_text="$3"
    local src
    local output_file="$TMP_DIR/$name.semantic.out"
    local status

    src="$(write_case "$name" "$source")"
    set +e
    "$ROOT_DIR/compiler" --semantic "$src" > "$output_file" 2>&1
    status=$?
    set -e

    if [[ $status -eq 0 ]]; then
        fail "$name" "expected semantic failure but command succeeded"
        return
    fi

    if ! grep -Fq "$expected_text" "$output_file"; then
        fail "$name" "expected semantic message [$expected_text], got [$(tr '\n' ' ' < "$output_file")]"
        return
    fi

    pass "$name"
}

expect_phase_contains() {
    local name="$1"
    local mode="$2"
    local source="$3"
    shift 3
    local src
    local output_file="$TMP_DIR/$name.phase.out"
    local pattern

    src="$(write_case "$name" "$source")"
    if ! "$ROOT_DIR/compiler" "$mode" "$src" > "$output_file" 2>&1; then
        fail "$name" "phase command $mode failed: $(tr '\n' ' ' < "$output_file")"
        return
    fi

    for pattern in "$@"; do
        if ! grep -Fq "$pattern" "$output_file"; then
            fail "$name" "missing pattern [$pattern] in phase output [$(tr '\n' ' ' < "$output_file")]"
            return
        fi
    done

    pass "$name"
}

expect_assembly_contains() {
    local name="$1"
    local source="$2"
    shift 2
    local src
    local asm="$TMP_DIR/$name.asm"
    local pattern

    src="$(write_case "$name" "$source")"
    if ! "$ROOT_DIR/compiler" "$src" > "$asm" 2>"$TMP_DIR/$name.asm.err"; then
        fail "$name" "assembly generation failed: $(tr '\n' ' ' < "$TMP_DIR/$name.asm.err")"
        return
    fi

    for pattern in "$@"; do
        if ! grep -Fq "$pattern" "$asm"; then
            fail "$name" "missing assembly marker [$pattern]"
            return
        fi
    done

    pass "$name"
}

expect_parser_failure() {
    local name="$1"
    local source="$2"
    local src
    local output_file="$TMP_DIR/$name.parse.out"
    local status

    src="$(write_case "$name" "$source")"
    set +e
    "$ROOT_DIR/compiler" "$src" > /dev/null 2>"$output_file"
    status=$?
    set -e

    if [[ $status -eq 0 ]]; then
        fail "$name" "expected parse failure but compile succeeded"
        return
    fi

    if ! grep -Fq "Invalid Expression" "$output_file"; then
        fail "$name" "parse failure message missing"
        return
    fi

    pass "$name"
}

expect_run "functions_and_locals" \
$'x=1\n\nfunc bump(a)\n\nx=a+1\nprintln(x)\nreturn x\n\nendfunc\n\nfunc greet()\nprintln("hello")\nendfunc\n\nprintln(bump(5))\ngreet()\nprintln(x)\n' \
$'6\n6\nhello\n1'

expect_run "expressions_and_identifiers" \
$'value_2=7\ncount10=(value_2+3)*2\nflag=(count10>10 and count10<30) or not(0)\nprintln(count10)\nprintln(flag)\n' \
$'20\n1'

expect_run "while_break_continue" \
$'i=0\nsum=0\nwhile(i<6)\ni=i+1\nif(i==2)\ncontinue\nendif\nif(i==5)\nbreak\nendif\nsum=sum+i\nendwhile\nprintln(sum)\n' \
$'8'

expect_run "for_loop" \
$'sum=0\nfor(i=0; i<5; i=i+1)\nsum=sum+i\nendfor\nprintln(sum)\n' \
$'10'

expect_run "arrays" \
$'array nums[3]\nnums[0]=10\ni=1\nnums[i]=20\nprintln(nums[0])\nprintln(nums[1])\n' \
$'10\n20'

expect_run "strings" \
$'msg="hello world"\nname=msg\nprint(name)\nprint("!")\nprintln("")\n' \
$'hello world!'

expect_run "int_input" \
$'x=read()\ny=read()\nprintln(x+y)\n' \
$'9' \
$'4 5\n'

expect_run "string_input" \
$'read(name)\nprintln(name)\n' \
$'hello world' \
$'hello world\n'

expect_run "negative_division" \
$'println(-7/2)\nprintln(-7%3)\n' \
$'-3\n-1'

expect_semantic_success "semantic_ok" \
$'func add(a,b)\nreturn a+b\nendfunc\n\narray nums[2]\nnums[0]=add(1,2)\nprintln(nums[0])\n'

expect_semantic_error "scope_error" \
$'if(1)\ntemp=5\nprintln(temp)\nendif\nprintln(temp)\n' \
"variable 'temp' is used outside its scope or before assignment"

expect_semantic_error "array_without_index" \
$'array nums[3]\nprintln(nums)\n' \
"array 'nums' must be indexed before use"

expect_semantic_error "index_non_array" \
$'x=5\nprintln(x[0])\n' \
"'x' is not an array"

expect_semantic_error "type_mismatch" \
$'x=10\nx="hello"\n' \
"cannot assign string to variable 'x' of type int"

expect_semantic_error "void_in_expression" \
$'func greet()\nprintln("hi")\nendfunc\n\nx=greet()\n' \
"void function 'greet' cannot be used in an expression"

expect_semantic_error "wrong_arg_count" \
$'func add(a,b)\nreturn a+b\nendfunc\n\nprintln(add(1))\n' \
"function 'add' expects 2 argument(s) but got 1"

expect_semantic_error "break_outside_loop" \
$'break\n' \
"'break' used outside of a loop"

expect_semantic_error "compile_time_oob" \
$'array nums[2]\nnums[2]=1\n' \
"array index 2 is out of bounds for 'nums' of size 2"

expect_runtime_error "div_zero" \
$'x=10\ny=0\nprintln(x/y)\n' \
"Division by zero"

expect_runtime_error "runtime_oob" \
$'array nums[2]\ni=2\nprintln(nums[i])\n' \
"Array index out of bounds"

expect_runtime_error "invalid_int_input" \
$'x=read()\nprintln(x)\n' \
"Invalid integer input" \
$'abc\n'

expect_runtime_error "invalid_string_input" \
$'read(name)\nprintln(name)\n' \
"Invalid string input"

expect_phase_contains "token_phase" "--tokens" \
$'println("hi")\n' \
"PRINTLN" \
"STRING(\"hi\")" \
"NEWLINE"

expect_phase_contains "ast_phase" "--ast" \
$'func add(a,b)\nreturn a+b\nendfunc\n\nprintln(add(2,3))\n' \
"Function name=add" \
"Main" \
"Println"

expect_phase_contains "symbols_phase" "--symbols" \
$'x=10\nmsg="hi"\narray nums[3]\n' \
"name=x type=int" \
"name=msg type=string" \
"name=nums type=int_array"

expect_assembly_contains "assembly_phase" \
$'println("hi")\n' \
"section .data" \
"section .text" \
"global main" \
"badintmsg db \"Invalid integer input\",0"

expect_parser_failure "empty_if_not_supported" \
$'if(1)\nendif\n'

printf '\nSummary: %d passed, %d failed\n' "$PASS_COUNT" "$FAIL_COUNT"

if [[ $FAIL_COUNT -ne 0 ]]; then
    exit 1
fi
