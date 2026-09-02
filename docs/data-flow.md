# Milk data flow

Milk's data-flow model is built into normal expression evaluation. There is no dedicated pipeline syntax.

The core rule is:

> A value can flow into an operation, and the result can continue flowing into the next operation.

## Assignment as a flow boundary

Milk uses `->` for assignment:

numbers -> [1, 2, 3]
result -> numbers * 2

The value produced by `numbers * 2` becomes the value of `result`.

Thinking about the same program as a flow:

[1, 2, 3]
    |
    v
  multiply by 2
    |
    v
[2, 4, 6]
    |
    v
  result

The syntax does not change. The runtime behavior supplies the flow semantics.

## Scalar flow

A scalar expression behaves normally:

value -> 10
result -> value + 5

The result is one scalar value.

## List flow

When one side of a binary operation is a list, the operation is applied to each flowing item.

numbers -> [1, 2, 3, 4]
result -> numbers * 3

This produces:

[3, 6, 9, 12]

The operation is conceptually:

1 * 3
2 * 3
3 * 3
4 * 3

## Two-list flow

Two lists flow together element-by-element.

left -> [1, 2, 3]
right -> [10, 20, 30]
result -> left + right

Conceptually:

1 + 10
2 + 20
3 + 30

The lists must have the same length.

## Function flow

Functions are natural transformation points in Milk's data-flow model.

make double(x)
    give x * 2

numbers -> [1, 2, 3, 4]
result -> double(numbers)

The runtime treats the call as four function evaluations:

double(1)
double(2)
double(3)
double(4)

and collects the outputs:

[2, 4, 6, 8]

## Scalar broadcasting

A scalar argument is reused for every flowing item.

make add(a, b)
    give a + b

numbers -> [1, 2, 3]
result -> add(numbers, 10)

Conceptually:

add(1, 10)
add(2, 10)
add(3, 10)

The result is:

[11, 12, 13]

## Multiple flowing inputs

When two or more list arguments flow into a function, they are zipped together.

make add(a, b)
    give a + b

left -> [1, 2, 3]
right -> [10, 20, 30]
result -> add(left, right)

Conceptually:

add(1, 10)
add(2, 20)
add(3, 30)

Different list sizes are an error because the runtime cannot form a complete element-by-element flow.

## Chaining transformations

Because function calls produce values, transformations can naturally be chained through assignments:

make double(x)
    give x * 2

make add_ten(x)
    give x + 10

numbers -> [1, 2, 3]
doubles -> double(numbers)
result -> add_ten(doubles)
show result

The flow is:

[1, 2, 3]
    |
    v
 double
    |
    v
[2, 4, 6]
    |
    v
 add_ten
    |
    v
[12, 14, 16]

## Why there is no pipeline operator

Milk does not currently need syntax such as `|>` to express a flow. Existing syntax already gives the runtime enough information.

The goal is for Milk to remain visually simple while the evaluation model becomes increasingly data-oriented.
