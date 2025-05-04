[Back to overview](main.md)

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

This type is also used for as the boolean value, with 0 being *false* and everything else being *true*.

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

Contains information about a field. The available information is:

| Name | Description |
| --- | --- |
| obstruction | This field cannot be seen through, passed through and players will be stuck inside |
| player | A player is loacated on this field |
| trapped | This field is trapped in some way |
| flammable | This field can be destroyed by fire |
| cover | This field will protect players from projectiles |
| shelter | This field will protect players from above |
| walkable | The player can move onto this field |

These properties of the field can be queried for, using the 'is' expression.

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

### Meta variables
**type**: int

Meta variables are used to get information about the games and the players state.

| Meta variable | Description |
| --- | --- |
| #x | Gets the players *x* position |
| #y | Gets the players *y* position |
| #id | Gets the players id |
| #board_x | Gets the board width |
| #board_y | Gets the board height |
| #*resource* | Where *resource* is the name of a resource. <br> Gets the amount of that resource the player has |

---
### Random int
**type:** int

**syntax:** `?`

**features:** random

Evaluates to a random non-negative integer.

---
### Random from set
**type:** *any*

**syntax:** `?(a ... z)`

**features:** random

Evaluates to one of the given values. All values must have the same type, which will also the the resulting type.

---
### Unary operation

These are operations on a single value. The operation symbol prefixes the value, as such:

*op* *value*

| Operation | Type | Description |
| --- | --- | --- | 
| - | int | Negates the integer value |
| ~ | int | Returns the opposite boolean value |

---

### Binary operation

These are operation on two values, with the operation symbol placed between the values:

*value* *op* *value*

#### Boolean operators
#### Equality operators
#### Comparision operators
#### Integer operators
#### Direction operators

---

### Read
**type:** int

**syntax:** `read`

**features:** ipc

Every field can contain an *int* value, which is set  using the `write` statement. `read` evaluates to the value set on the field the player is standing on.

---

### Pager Read
**type:** int

**syntax:** `pager_read`

**features:** ipc

Every player is equiped with a pager. It can the set a channel using `pager_set`, and messages are send on the channel using `pager_write`. Received messages can be read using `pager_read`, starting with the oldest message. Read a message also removes it from the pager.

If there is no new messages, this evaluates to `0`.

---

### Scan
**type:** field

**syntax:** `scan d i`

Takes a snapshot of the properties of the field in direction `d`, at a distance of `i` fields. It is possible to scan past an obstruction.

---


### Look
**type:** int

**syntax:** `look` *dir* *property*

Evaluates to the distance to the nearest field in the given direction, which has the given property.

---

### Is
**type:** int

**syntax:** *field* `is` *property*

Evaluates to `1` if the given field has the given property. Otherwise it evaluates to `0`.

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

### Label
**syntax:** *name*`:`

A label gives a particular point in the program a name. It can be used to jump to that point during execution. The names follow the same syntax as variables, but must be unique.

---

### GoTo
**syntax:** `goto` *name*

Continues execution of the program at the *name* label.

---

### Declaration
**syntax:** *type* *name* `;` | *type* *name* `=` *expr* `;`

**features:** memory

Declares a variable of the given type and name, assigning the resulting value of evaluating the expression, if one is given.

---

### Assignment
**syntax:** *name* `=` *expr* `;`

**features:** memory, sugar*

Assigns the resulting value of evaluating the expression, to the variable of the given name.

---

### Increment/Decrement
**features:** memory, sugar

### If-Else
**features:** control

### If-Is
**features:** control, sugar

### Repeat
**syntax:** `repeat` *x* *stmt*

Where *x* is a constant integer. Compiles the given statement *x* times 

---

### While
**features:** loops

### Break
**features:** loops

### Continue
**features:** loops

### Wait <sub><small>action</small></sub>
**syntax:** `wait;`

Spend an action doing nothing.

---

### Pass
**syntax:** `pass;`

Ends the current turn.

---

### Move <sub><small>action</small></sub>
**syntax:** `move` *dir* `;`


Moves the player one field in the given direction, if that field is walkable, unobstructed and in bounds of the board.

---

### Trench <sub><small>action</small></sub>
**syntax:** `trench d?`

### Fortify <sub><small>action</small></sub>
**syntax:** `fortify d?`

### Write
**features:** ipc

**syntax:** `write i`

### Pager Write
**features:** ipc

**syntax:** `pager_write i`

### Pager Set
**features:** ipc

**syntax:** `pager_set i`

### Collect
**syntax:** `collect d`

### Say
**syntax:** `say i`

### Shoot <sub><small>action</small></sub>
**themes:** military

**syntax:** `shoot d`

### Bomb <sub><small>action</small></sub>
**themes:** military

**syntax:** `bomb d i`

### Mine <sub><small>action</small></sub>
**themes:** military

**syntax:** `mine d`

### Disarm <sub><small>action</small></sub>
**themes:** military

**syntax:** `disarm d?`

### Plant Tree <sub><small>action</small></sub>
**themes:** forestry

**syntax:** `plant_tree d`

### Chop <sub><small>action</small></sub>
**themes:** forestry

**syntax:** `chop d`

### Wall <sub><small>action</small></sub>
**themes:** forestry

**syntax:** `wall d`

### Bridge <sub><small>action</small></sub>
**themes:** forestry

**syntax:** `brdige d`

### Meditate <sub><small>action</small></sub>
**themes:** wizardry

**syntax:** `meditate`

### Fireball <sub><small>action</small></sub>
**themes:** wizardry

**syntax:** `fireball d`

### Freeze <sub><small>action</small></sub>
**themes:** wizardry

**syntax:** `freeze d i`

### Dispel <sub><small>action</small></sub>
**themes:** wizardry
**syntax:** `dispel d`

### Mana Drain <sub><small>action</small></sub>
**themes:** wizardry

### Projection <sub><small>action</small></sub>
**themes:** wizardry



[Back to overview](main.md)