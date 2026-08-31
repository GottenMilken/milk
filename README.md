Milk is a minimalistic programming language designed to be incredibly simplistic and human readable.
As of now, it is in incredibly early stage and only exists to use as the baseplate for a more "fleshed out" version of milk.

> [!NOTE]
> Milk is still very early, so there are not many commands. For a list of commands, see below.

show - Equivalent of print in python.
example
> Input:
> show "Hello, World!"

> Output:
> Hello, World!

> [!TIP]
> You can use show to print variables, like this. show "Welcome to milk, {name}!"

Speaking of variables, you define a variable using an arrow symbol (->)
example
> Input:
> name -> "gottenmilken"
> show name

> Output:
> gottenmilken

>[!TIP]
> variables can be any type, string, boolean, integer etc. you don't need to specify the type

check - Equivalent of if in python.
otherwise - Equivalent of else in python
example
> Input:
> count -> 6
> check count == 6
>  show "Count is 6"
> otherwise
>  show "Count is less/more than 6

> Output:
> Count is 6

>[!TIP]
> There is also < etc.

during - Equivalent of while in python
example
> Input:
> count -> 1
>
> during count < 6
>   show count
>   count -> count + 1
> during count <= 5

