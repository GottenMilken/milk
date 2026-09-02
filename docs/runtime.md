# Runtime and errors

Milk reports errors to standard error and exits with a non-zero status when parsing or evaluation fails.

## File errors

Running Milk without exactly one source file prints usage information.

If the source file cannot be read, Milk reports an error and exits with status 1.

## Parse errors

Parse errors include a source location:

Milk error at line:column: message

Examples of parser failures include malformed function calls, missing block indentation, and invalid statements.

## Runtime errors

Examples include:

Milk error: unknown value 'name'
Milk error: unknown function 'work'
Milk error: function 'add' expects 2 arguments, got 1
Milk error: cannot divide by zero
Milk error: data flow needs equal list sizes

## Type errors

Milk is dynamically typed. Operations validate the values they receive at runtime.

Examples:

- subtraction requires numbers
- multiplication requires numbers
- division requires numbers
- modulo requires numbers
- ordering comparisons require numbers
- string concatenation uses `+` with two strings

## Function errors

A function call must provide exactly the number of arguments declared by the function.

`give` may only be used inside a function.

A function that reaches the end without `give` produces `empty`.

## Data-flow errors

Two flowing list inputs must have the same length:

left -> [1, 2]
right -> [10, 20, 30]
result -> left + right

This produces:

Milk error: data flow needs equal list sizes

The same rule applies to function calls with multiple list arguments.
