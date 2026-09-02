# Getting started

This guide gets a small Milk program running from source to output.

## Build Milk

From the repository root:

make

The build creates an executable named `milk`.

To remove build output:

make clean

## Run a program

Milk expects one source file:

./milk program.milk

For example:

name -> "Milk"
show "Hello, {name}!"

Run it with:

./milk program.milk

Check the installed interpreter version with:

./milk --version

## Shell commands

Use `run` followed by a quoted command to execute it through the system shell. The command output is written directly to the terminal:

run "echo Hello from Milk"

If the shell command exits with a non-zero status, Milk reports a runtime error and the program fails.

## Variables

Use `->` to put a value into a variable:

name -> "Milk"
age -> 21
ready -> yes
nothing -> empty

Milk does not currently require explicit type declarations. A variable receives whatever value the expression produces.

Variables can be used in later expressions:

score -> 10
bonus -> 5
total -> score + bonus
show total

## Outputt

Use `show` to evaluate and display a value:

show "Hello"
show 42
show yes
show empty

Strings can contain variable interpolation using braces:

name -> "Milk"
show "Hello, {name}!"

## Blocks

Milk uses indentation to define blocks.

check age >= 18
    show "adult"
otherwise
    show "minor"

Keep indentation consistent within a block.

## A first function

Functions use `make` and return a value with `give`:

make add(a, b)
    give a + b

result -> add(10, 20)
show result

Output:

30

## A first data-flow program

The most important current direction in Milk is data flow.

make double(x)
    give x * 2

numbers -> [1, 2, 3, 4]
result -> double(numbers)
show result

Output:

[2, 4, 6, 8]

A list reaching a function parameter causes the function to run once per item. This lets ordinary Milk functions act as transformations without adding pipeline syntax.

## Where to go next

Read the [language reference](language.md) for syntax and behavior, then read [data flow](data-flow.md) for the execution model behind list transformations.
