[Back to overview](../README.md)

# The trench language

The trench langauge is used to describe the behavior of characters. It is executed from the top and down.

A program consists of a series of statements, which may consist of other statements and expressions, all of which are descriped in this document.

Some language constructs require some game settings to be enabled, called features and themes. If a language construct is marked by some themes, it is enabled if one or more of those themes are enabled. On the other hand, if a language construct is marked by some features, they must all be enabled. 

## Comments

Comments start with `//` and end at the end of the same line.

Examples:
```
// This entire line is a comment

int x = 2;  // There can be actual code before a comment
```

## Types

### int

Whole numbers including negatives, represented using 32 bits. Only base-10 is currently supported.

This type is also used for as the boolean value, where positive values are *true*, and other values are *false*.

| true | false |
| --- | --- |
| x > 0 | x <= 0 |

Examples:
```
-13
0
1
6142
```

### dir

The 4 cardinal directions, N, S, E and W.

Examples:
```
N
S
E
W
```

### field

A value of the `field` type is a collection of properties describing a specific field.

The available properties are:

| Name | Description |
| --- | --- |
| obstruction | This field cannot be seen through, passed through and players will be stuck inside |
| player | A player is loacated on this field |
| trapped | This field is trapped in some way |
| flammable | This field can be destroyed by fire |
| cover | This field will protect players from projectiles |
| shelter | This field will protect players from above | 
| empty | This field is empty |
| trench | This field is a trench |
| ice_block | This field is an ice block |
| tree | This field is a tree |
| ocean | This field is an ocean |
| wall | This field is a wall |
| bridge | This field is a bridge |
| clay | This field is a clay pit |
| mountain | This field is a mountain |
| enemy | There is an enemy on the field |
| ally | There is an ally on the field |
| vehicle | There is a vehicle on the field |
| all | A field with every property |

A field value with a single property can be created with the syntax: `@`*property*.

Values of this type can be used with the following operators.

| Operator | Result | Type |
| --- | --- | --- |
| *a* + *b* | The union of the properties of the operands | field |
| *a* - *b* | The properties of the left hand operand, except the properties of the right hand operand | field |
| *a* == *b* | 1 if the operands are entirely equal, otherwise 0 | int |
| *a* != *b* | 1 if the operands are at all different, otherwise 0 | int |
| *a* is *b* | 1 if the left hand operand has atleast all the properties of the right hand operand, otherwise 0 | int |
| *a* any *b* | 1 if the operands have any property in common, otherwise 0 | int |
| ! *a* | The field with the opposite properties of the operands | field |



### resource

The `resource` type describes the different resources that a player can have, such as wood and bullets. The syntax is a hashtag followed by the name of the resource, e.g. `#wood` or `#explosive`. Players can use the builtin [count](./builtins.md#count) to get their current amount a resource.

The available properties are:

- explosive
- ammo
- mana
- sapling
- clay
- wood
- bear_trap
- metal

### array

Arrays are fixed sized lists of values belonging to a single type. The size is given at declaration, and values can be indexed, starting at 0. Arrays cannot be given as argument to a function, nor can they be the return type.

```
// Declare
int[2] my_array;

// Assign
my_array[0] = 57;

// Access
say(my_array[0]);
```

### function

Functions are useful for encapsulating logic. The trench language contains [builtin](./builtins.md) functions, which lets players interact with the game, and players can also define their own functions in their programs.

The type of a function is written as such:

```type(type, ...)```

and a function is specified as such:

```type:(type name, ...) { ... }```

The first type is the return type, all functions must have one. Inside the parentheses are the function parameters, which there may be zero or more of, and finally the function body.

Examples:

```
int(int,int) add = int:(int a, int b) {
    return a + b;
};

let move = int:(dir d) {
    if (!look(d, @trapped) = 1) move(d);
};

let random_move = int:() {
    move(?[N S E W]);
};


```

Notice that even though all function have a return type, not all of these examples have a return statement. This is because all functions have an implicit return statement at the end, returning the default value of their return type.

Inside of a function only the parameters, locally declared and global variables are available. Additionally the local variable `this` is implicitly declared in all functions, and refer to the function itself, enabling recursion.

```
let a = 0;

let f1 = int:(int b) {

    let c = 0;

    let f2 = int:(int d) {
        let e = 0;
        // can reach: a, d, e
    };

    let f = 0;

};

let g = 0;

```

If a variable of the function type is declared, but it is not assigned a function it will get the `null` value. Players can check for this case as in the example below.

```
int() f;

if (f != null)
    f();
```

Functions have a few pieces of syntactic sugar. 
- If a return type is not specified, the return type will be `int`. 
- If the function should just return the result of an expression the `=>` synax can be used:

```
let sqr = :(int a) => a * a;

let negate = :(int a) {
    if (a < 0) return a;
    else return -a;
};

let opposite = dir:(dir d) => d << 2;
```


## Expressions

### Variables

**features:** memory

Variables are name for data, they start with a letter which is followed by an amount of letters, numbers and underscores. 

Examples:

```
a
my_variable
MyVariable
field2_1
```
Variables do not have to be unique. When referenced the nearest variable in scope will be used.

```
int x = 1;
int x = 2;
{
    say x;     // -> 2
    int x = 3
    say x;     // -> 3
}
say x;         // -> 2
```

---
### Random int
**type:** int

**syntax:** `?`

**features:** random

Evaluates to a random non-negative integer.

---
### Random from set
**type:** *any*

**syntax:** `?[a ... z]`

**features:** random

Evaluates to one of the given values. All values must have the same type, which will also the the resulting type.

---
### Unary operation

*op* *value*

| Operation | Operand | Description |
| :---: | --- | --- | 
| - | int | Negates the integer value |
| ! | int | Returns the opposite boolean value |
---

### Binary operation

*value* *op* *value*

| Left | Operation | Right | Description |
| --- | :---: | --- | --- | 
| int | + | int | Addition |
| int | - | int | Subtraction |
| int | / | int | Division |
| int | % | int | Modulo |
| int | & | int | Logical AND |
| int | \| | int | Logical OR |
| * | == | * | Equality |
| * | != | * | Inequality |
| int | < | int | Less than |
| int | <= | int | Less than or equal |
| int | > | int | Greater than |
| int | >= | int | Greater than or equal |
| dir | << | int | Rotate left |
| dir | >> | int | Rotate right |


---

### Increment/Decrement
**type:** int

**syntax:** `++`*int*,  *int*`++`, `--`*int*,  *int*`--`

**features:** memory, sugar

This can only be called on variables.

| | |
| ---  | --- |
| `++x` | Increases the value of *x* by 1, and evaluates to the increased value | 
| `x++` | Increases the value of *x* by 1, and evaluates to the prior value |
| `--x` | Decreases the value of *x* by 1, and evaluates to the decreased value | 
| `x--` | Decreases the value of *x* by 1, and evaluates to the prior value |

---

## Statements

### Expression

**syntax:** *expr*`;`

Expression can be used as statements. This will evaluate the expression, and discard the result. As examples this can be used to call a function, where the returned value is irrelevant, or to increment or decrement an integer.

Examples

```
move();

i++;

2 + 2;
```

### Label
**syntax:** *name*`:`

A label gives a particular point in the program a name. It can be used to jump to that point during execution. The names follow the same syntax as variables, but must be unique.

---

### GoTo
**syntax:** `goto` *name*

Continues execution of the program at the *name* label.

---

### Declaration
**syntax:** *type* *name* `;` | *type* *name* `=` *expr* `;` | *let* *name* `=` *expr* `;`

**features:** memory

Declares a variable of the given type and name, assigning the resulting value of evaluating the expression, if one is given.

---

### Assignment
**syntax:** *name* `=` *expr* `;`

**features:** memory, sugar*

Assigns the resulting value of evaluating the expression, to the variable of the given name.

---

### If-Else
**features:** control

```

if (look(N, @ocean) > 10)
    give_up();

if (#mana == 0) meditate();
else fireball(d);
```

### If-Is
**features:** control, sugar

```
if (x) 
    is 1 move(N);
    is y move(S);
    is (2 * y) move(E);
else
    say(0);
```

### Repeat
**syntax:** `repeat` *x* *stmt*

Where *x* is a constant integer. Compiles the given statement *x* times 

```
repeat 5 move(N);

repeat 3 {
    move(N);
    trench();
}
```

---

### While
**features:** loops

```
while (x) {
    do_stuff();
}


while (x) : (x--) {
    do_stuff();
    if (x <= 0) break;
    else continue;
}
```

---

### Return

**syntax:** `return` *expr*`;`

Is only allowed inside functions, and the return type must match the function.


---


[Back to overview](../README.md)