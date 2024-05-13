# trench
Programming PvP game

## File extentsion
| | |
| --- | --- |
| .tr | Source code for a players behavior |
| .trc | Compiled .tr file |
| .trg | A game file |

## Trench langauge

A program consists of a register block and a directive block.

### Register block
Here the set of available names has to be defined. For example if you want to be able to save information under the name 'a', a register 'a' must be defined here.

```
[const a := 1, b, counter := 0, ...]
```

A register can be given an explicit initial value, by assignment, otherwise it will take 0 as its value. The 'const' modifier, makes the register unmodifiable at run time.

Players have 4 default registers:
| register | explaination |
| --- | --- |
| x | A constant register, containing the players x position |
| y | A constant register, containing the players y position |
| bombs | A register, containing the players remaining bombs |
| shots | A register, containing the players remaining shots |

### Directive block
Here the logic is defined. It consist of a series of statements, which will be interpreted from the top. Statements take some steps to execute, a limit to how many steps can be taken in a turn is set in the game rules. Executing some statements will use an action, the number of actions available per turn is defined by the game rules.

| expression | explaination | examples |
| --- | --- | --- |
| _x_ | reference to a register 'x' | counter |
| _i_ | interger value | 14, -1 |
| _d_ | cardinal direction | N, E, S, W |
| _a_ _binop_ _b_| binary operation | 1 + 2, x % 10 |
| _unop_ _a_ | unary operation | ~a |
| scan | get the distance to the closest trench in some direction, or 0 if there is none | b := scan N; |

binary operators: +, -, *, /, %, >, >=, <, <=, !=, =, |, &

unary operators: ~ (not)

| statement | explaination | examples | actions |
| --- | --- | --- | --- |
| if | conditional execution | if (c) { ... } else { ... } , if (c) { }  | 0 |
| block | A contained series of statements. | { ... } | 0 |
| assign | Assign a value to a register. | a := a + 1;, a +:= 1;| 0 |
| label | Marks a location in the directive | loop: | 0 |
| move | Move the player 1 space in some direction. | move N; | 0 |
| expand | Dig a trench in some direction. | expand E; | 1 |
| shoot | Shoot in some direction, from the players current position. Any player in that direction, not in a trench will die. | shoot S; | 1 |
| bomb | Throw a bomb in some direct, some number of spaces. It explodes after 1 round, killing any player hit, unless they are in a fortified trench. Fortified trenches become unfortifed, unfortified trenches are destroyed. | bomb N (2+1); | 1 | 
| fortify | Fortify the current location, if there is a trench. | fortify; | 1 |
| trench | Dig a trench at the current location. | trench; | 1 |
| wait | Use an action on nothing. | wait; | 1 |
| pass | End the current turn. | pass; | 0 |
| goto | Jump to a location in the directive. | goto loop; | 0 |

## Game file
The game file specifies the players and rules of a particular game. It consists of key value pairs, seperated by semi-colons, defining a set of values.

| key | explaination |
| --- | --- |
| bombs | the number of bombs a player can throw in a game |
| shots | the number of shots a player can fire in a game |
| actions | the number of actions a player can take per turn |
| steps | the number of steps a player can can perform per turn |
| change | the number of round that must pass before players can change directive, 0 meaning never |
| player | the id, x and y position, and source file of a player |
| board_x | the board width |
| board_y | the board height |
| nuke | wheter or not every space on the board blows up, before directive changes occur |

In the following, every value is set to its default, except the player:
```
bombs:10;
shots:100;
actions:1;
steps:1000;
change:0;
nuke:0;

board_x:20;
board_y:20;

player:1,5,5,./player1.tr;
player:2,8,9,./player2.tr;

```