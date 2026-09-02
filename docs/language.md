# Milk language reference

This document describes the currently implemented Milk syntax.

## Source files

A Milk source file is a sequence of statements. Statements are separated by newlines.

Blocks are defined by indentation after a block-opening statement.

check yes
    show "inside"

show "outside"

## Values

### Numbers

Numbers are written directly:

count -> 10
ratio -> 2.5

Milk stores numeric values as floating-point numbers and prints them with a compact representation.

### Strings

Strings use double quotes:

message -> "Hello, Milk"

String interpolation can reference a variable:

name -> "Milk"
show "Hello, {name}!"

An interpolation name is resolved as a variable. Unknown interpolation names remain as written in the string.

### Booleans

Milk uses `yes` and `no`:

ready -> yes
finished -> no

### Empty

The `empty` value represents the absence of a value:

value -> empty
show value

### Lists

Lists use square brackets and comma-separated values:

numbers -> [1, 2, 3, 4]
names -> ["Milk", "World"]

List items can be expressions:

base -> 10
numbers -> [base, base + 1, base + 2]

## Variables

Assign a value with an arrow:

name -> "Milk"
score -> 10 + 5

A variable reference is just its name:

show name
show score

Assignments create or replace a variable in the current environment.

## Shell commands

`run` is an expression that executes a command through the system shell and returns its standard output as a string:

```milk
os -> run "cat /os/release"
show os
```

The command's trailing newline characters are removed, similar to shell command substitution. Standard error remains attached to Milk's standard error. A non-zero command status causes Milk to report a runtime error.

Because `run` is an expression, its result can be assigned, passed to functions, compared, or displayed:

```milk
command -> "echo hello"
output -> run command
show output
```

## Output

`show` evaluates one expression and prints the resulting value:

show "Hello"
show 10 + 20
show [1, 2, 3]

Lists are printed using square brackets and comma-separated values.

## Arithmetic

Milk currently supports:

| Operator | Meaning |
| --- | --- |
| `+` | Addition or string concatenation |
| `-` | Subtraction |
| `*` | Multiplication |
| `/` | Division |
| `%` | Remainder |

Examples:

sum -> 10 + 5
product -> 4 * 3
half -> 10 / 2
rest -> 10 % 3
message -> "Hello, " + "Milk"

Multiplication, division, and remainder bind more tightly than addition and subtraction.

Milk currently parses expressions without parentheses for grouping. Function calls use parentheses.

## Comparisons

Milk currently supports:

| Operator | Meaning |
| --- | --- |
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `<=` | Less than or equal |
| `>` | Greater than |
| `>=` | Greater than or equal |

Examples:

check score >= 10
    show "ready"

check name == "Milk"
    show "found"

Numeric ordering comparisons require numbers. Equality can compare Milk values generally.

## Conditionals

Use `check` for a conditional block:

age -> 21

check age >= 18
    show "adult"

Use `otherwise` for a fallback:

check age >= 18
    show "adult"
otherwise
    show "minor"

An `otherwise check` chain is supported:

check score >= 90
    show "A"
otherwise check score >= 80
    show "B"
otherwise
    show "C"

## Fixed repetition

`repeat` evaluates its count and runs its block that many times:

count -> 1

repeat 5
    show count
    count -> count + 1

The repeat count must be numeric. A non-integer count is truncated by the current runtime behavior.

## Conditional loops

`during` repeats while its condition evaluates to a true boolean:

count -> 1

during count <= 5
    show count
    count -> count + 1

If the condition is not a boolean, Milk reports a runtime error.

## Functions

Define a function with `make`:

make greet(name)
    show "Hello, {name}!"

Call it with parentheses:

greet("Milk")

Functions can accept multiple parameters:

make add(a, b)
    give a + b

### Returning values

Use `give` to return a value:

make add(a, b)
    give a + b

result -> add(10, 20)
show result

A function that finishes without `give` returns `empty`.

### Local scope

Function parameters and assignments are local to the function. Functions can read variables from their parent environment.

name -> "global"

make test(value)
    local -> value
    show local
    show name

test("local")
show name

An assignment inside the function does not replace the global variable with the same name.

### Recursion

Functions can call themselves:

make factorial(n)
    check n <= 1
        give 1
    give n * factorial(n - 1)

show factorial(5)

## Data flow

Lists participate directly in expression evaluation.

For a binary operation, if either side is a list, Milk applies the operation across the flowing values.

A list and scalar:

numbers -> [1, 2, 3]
result -> numbers * 10
show result

Output:

[10, 20, 30]

Two lists flow element-by-element and must have equal sizes:

left -> [1, 2, 3]
right -> [10, 20, 30]
result -> left + right
show result

Output:

[11, 22, 33]

A scalar can be broadcast across a list:

numbers -> [1, 2, 3]
show numbers + 10

## Function data flow

When a function call contains a list argument, Milk evaluates the function once for each list item.

make double(x)
    give x * 2

numbers -> [1, 2, 3, 4]
result -> double(numbers)
show result

Output:

[2, 4, 6, 8]

Scalar arguments are broadcast:

make add(a, b)
    give a + b

numbers -> [1, 2, 3]
result -> add(numbers, 10)
show result

Output:

[11, 12, 13]

Multiple list arguments flow together element-by-element:

make add(a, b)
    give a + b

left -> [1, 2, 3]
right -> [10, 20, 30]
show add(left, right)

Output:

[11, 22, 33]

List arguments with different sizes are rejected.

## Statements versus expressions

Some Milk constructs are statements:

show value
check yes
    show "yes"
repeat 3
    show "again"

Expressions produce values and can be assigned, passed to functions, compared, or displayed:

result -> add(10, 20)
show result * 2

Function calls are expressions, so their return values can continue flowing through the program.

