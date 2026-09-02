# Examples

The repository contains runnable examples in `examples/`.

## Hello world

show "Hello, World!"

## Variables and interpolation

name -> "Milk"
age -> 21
show "Welcome to {name}!"
show age

## Arithmetic

score -> 10 + 5 * 2
half -> score / 2
show score
show half

## Conditional logic

value -> 5

check value >= 10
    show "above 10"
otherwise
    show "below 10"

## Repetition

count -> 1

repeat 5
    show count
    count -> count + 1

## While-style loops

count -> 1

during count <= 5
    show count
    count -> count + 1

## Functions

make greet(name)
    show "Hello, {name}!"

greet("Milk")

## Function results

make add(a, b)
    give a + b

result -> add(10, 20)
show result

## Recursion

make factorial(n)
    check n <= 1
        give 1
    give n * factorial(n - 1)

show factorial(5)

## Data flow through a function

make double(x)
    give x * 2

numbers -> [1, 2, 3, 4]
result -> double(numbers)
show result

## Data flow through arithmetic

numbers -> [1, 2, 3, 4]
result -> numbers * 3
show result

## Two-list flow

make add(a, b)
    give a + b

left -> [1, 2, 3]
right -> [10, 20, 30]
show add(left, right)

## Mixed scalar and list flow

make add(a, b)
    give a + b

numbers -> [1, 2, 3]
result -> add(numbers, 10)
show result

## Running repository examples

From the repository root:

./milk examples/printtest.milk
./milk examples/checks.milk
./milk examples/repeat.milk
./milk examples/during.milk
./milk examples/functions.milk
./milk examples/dataflow.milk
