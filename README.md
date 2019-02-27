# Xirita
This is an interpreter made in C for the Xirita language, a language made for fun. The project is still in development and is working on a substet of the language.

## The interpreter XRun

The interpreter is done only with the C standard library and the OS C libraries and has no further dependences. No tooling is used at all. . The interpreter is compiled using make and is designed to compile mostly on linux with compativility on Windows only on major releases.

This interpreter has implemented with the help of the Dragon Book and it's implementation is divided in four modules:
* A lexer, witch contains some extra options for handling blocks, empty lines, and such. Does not abstract away spaces since spaces and tabs are relevant on the language.
* A recursive descendant parser. The grammar of such parser is specified as a comment in the source file. The parser also has some backtracking to handle special cases. The parser generates an AST.
* The sdt_run witch interprets and executes the AST. This is done in two phases, one in witch function declarations are imports are made (parses the whole project) and another where this is run.
* An auxiliar data structures library that includes a generic type variable, lists, hashtables and AST. This is implemented in a OOP fashion.

## The language
The language is structured and imperative and may use some constructs that simulate OOP and functional paradigms. It has a dynamic and duck typing with no type coersion (similar to python). It automatically allocates and deallocates memory but it doesn't usa garabage collection and it doesn't allow globals of any kind, memory is scope based and is only deallocated when returning from function call. The language doesn't have side effects except for those that may be introduced in special cases using IO. The language includes some basic form of pattern matching and error handling.

Example (Simple Recursive Fibbonaci Series):
````Xirita
[FIB]
n   ARGUMENT 0
    ASSIGN n, EQ 1, THEN: 1, RETURN
    ASSIGN n, EQ 2, THEN: 1, RETURN
a   ASSIGN n, SUB 1, FIB a
b   ASSIGN n, SUB 2, FIB b
    SUM a b

# Alternative
[FIB 1|2] 1
[FIB n] SUM { FIB n-1 } { FIB n-2 }
````

### Syntax
The language has two type of names, variables and functions. Variables are formet exclusively by lowercase letters without spaces or slashes of any kind, while function names are uppercase. This allows to distiguish at any point what is a function and what is not. Other lexemes that may be found are literals and operators. Line comments are prefixed with symbol #.

### Semantic
A function is declared using []. The definition may either be inline, in witch case it finishes on the next line, or a block. In this second case the block needs to have an indeentation superior to the one used on the declaration of the function. Notice that if a function call is writen outside any block it falls on the global block with indeentation 0. Example:
````
BLOCK 0
[DECL BLOCK 0] INLINE
[DECL BLOCK 0]
    BLOCK 1
    [DECL BLOCK 1]
        [DECL BLOCK 2] INLINE
        BLOCK2
BLOCK 0
````

A line is composed of one or more function calls. In case of being a single call it is of the form: `v   FUNCTION a1 a2 a3`, where 'v' is the variable and a1 a2 a3 is the argument list. From the scope of the call a variable is read-write while arguments are read only. In this language all functions are modifyiers, that is, all functions take a variable and modify it. If no variable name is specified the variable used is the empty word (also called ghost). The empty word is automaticaaly initialized to the value of the variable on each function call but the value of the variable can by accessed at any point within the function (with constant value) using VARIABLE. Example:
````
[FUNCTION]
a   ASSIGN 66
b   ASSIGN 10
b   ADD a
    [FUNCTION2]
        ADD 5
        PRINT
b   FUNCTION2
````

Thanks to this property where the same name is both the input and the output, function calls may be concatenated if using the same variable with the operator ','. So, for instance the last example would be:
````
[FUNCTION]
a   ASSIGN 66
b   ASSIGN 10, ADD a
    [FUNCTION2] ADD 5, PRINT
b   FUNCTION 2
````
Notice that no RETURN is requiered, the value of the last variable name specified is the one that will be returned.

### Blocks
We have already defined what constitutes a block and mentioned that function declarations may open a new block. New blocks may also be opened using the operator ':' at the end of a function call. This may be typically used for control structures. This however are still functions, and any non inbuild function may open specify when to call the contents of a new block using the inbuild function BLOCK. (Examples on base.x file).

### Types
The types defined in this language as of today are Number, String, Boolean, List and Function. The user is not allowed to define new types. On revews of the spec types Array and Map may be introduced. The type may change at any time (just like the value). The type of a variable may be checked with the function TYPE.

The container types (List) are always mutable. That is tough the object is the same it's contents are only references and may be modified even when the object is passed as an argument instead of a variable.

### Namescpaces and function signatures
All functions are mapped to a global table. In case of name collision the last function declared replaces the old one (making the old one usually uncallable). Function names don't allow the use of spaces or underscores, however in order to improve organization namespces are introduced. A namespace is a prefix separated by a space to the function name, for example in `[CASTLE SOLDER]` SOLDER is the function name and CASTLE a namespace. Namespaces may be concatenated. On function declarations inside a function a the father automatically becomes the namespace of the child. As such:
````
[CITY HALL PLANT] ASSIGN "Beautiful"
# Is the same as
[CITY]
    [HALL]
        [PLANT]
            ASSIGN "Beautiful"
````

When codified on the global function table each function declaration uses as key a signature. The signature is constructed appending the function name with all it's namespaces with underscore, as such in the code avobe the signature would become `CITY_HALL_PLANT_`. A function may also have arguments, arguments may be accessed with the function `ARGUMENT n`, however for convenience they may also be declared on the function declaration and will be automatically initialized at runtime, for instance `[FUNCTION a b c]`. Notice that arguments may appear at any space withing the function declaration and as a suffix, for instance `[WORLD myworld FUNCTION a b c]`, this can be typacally used to emulate OOP. Function signatures of this kind of declarations are of the form 'WORLD_vFUNCTION_vvv' (where 'v' symbol stands for variable name) and they are called generic signatures becouse they can distinguish difierent function declarations based on number and position of arguents alone without any pattern matching.

### Pattern matching (Incomplete)
A function declaration may also include literals, of witch there are only literals for types Number, String and Boolean. As for my implementation these are encoded on the function signature such [FIB 1] = 'FIB_li1%' where 'l' stands for literal, 'i' for type integer (or number) 1 is the codification and % marks the end of the codification. On pattern matching first a common lookup is done for the generic signature (constant time using a hash table) and then a more advanced lookup is done to match with the pattern. Some abiguity may appear and it is resolved using the algorithm specified on the sdt_run.c file with name "sdt_resolve_pattern_matching".

It's not implemented but it's planned that the operator '|' (without spaces) may be used to specify multiple options for a literal as as such have multiple signatures pointing to the same definition. 

### Subexpressions (Not Yet Implemented)
One problem of the language is that it may be way to vertical. That is becouse every use of a diferent variable requieres a new line. This on the long run may become very peddantic. To solve this subexpressions are introduced. A subexpression may replace any argument. A subexpression may be consideres syntaxis suggar for:

````
a   FIB n-1
b   FIN n-2
    SUM a b
# Using subexpressions instead
    SUM { FIB n-1 } { FIB n-2 }
````

As the example shows a subexpression works on the same scope (has acces to the same variables) that the rest of the function body, however the variable used in the subexpression is a copy of the variable used on the line and will be used after its evaluation as the argument that the subexpression replaces.

Another kind of subexpression is a mathematical expression, witch is considered a subexpression witch definition is expressed in another language, ex: n-1 (Not implemented).

Notice that subexpressions are always inline only.

### Functional features
I've previously stated that all functions make use of a variable. In case of function declarations those also may make use of a variable, in this case the variable is initialized as the function that's been declared on that line. Letter that function may be called using the CALL f a1 a2 a3 function, where the f is the variable holding the function to be called. Example:

````
[MAP f] IN { VARIABLE }: CALL f
[MAIN]
add1  [ADD1] ADD 1
      LIST 1 2 3, MAP add1, PRINT
````

Just like we may have an empty name for variables, the same may happen for functions (anonymous functions). Those are uncallable unless they are stored in a variable but may be very useful as lambda expressions. Example:

````````
add1  [] ADD 1
      LIST 1 2 3, MAP add 1, PRINT
# Or like a lambda
      LIST 1 2 3, MAP { [] ADD 1 }, PRINT
````````

### Other details (Not Yet Implemented)
One could think that if a line uses the empty word as a function name that would call such function from the function table. However for convenience I've decided that in this special case the buildin function ASSIGN is called. This function is mainly used to assign literal values to variables. In the special case that the number of arguments is grater than one ASSIGN behaves like LIST. With no arguments the function fails. This is what allows the expression `[FIB 1|2] 1` to work.

As for error handling, if a function FAIL is called, the interpreter will ignore all further calls until a function FAILED is seen, witch will open a block that would be executed only in case it was reached in this state. In this case it will also assign the stack trace to the variable. It is planned that FAIL and FAILED may use arguments to distinguish diferent types of errors without needing to consult the trace.
