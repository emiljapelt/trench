```
╶┬╴                                 
 │ ┌╴ ┌─┐         ╷                 
 │ │  │ │         │                 
 │ ╵  ├─┘     ┌─╴ ├─┐               
 ╵    │   ╷   │   │ │               
      └─╴ ├─┐ └─╴ ╵ ╵               
          │ │                       
          ╵ ╵            
```
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
[int a = 1, b, counter = 0, d = N, dir k, ...]
```

A register can be given an explicit initial value, by assignment, otherwise it will take 0 as its initial value. By default register carry intergers, but they can also be made to carry directions.

### Directive block
Here the logic is defined. It consist of a series of statements, which will be interpreted from the top. Statements take some steps to execute, a limit to how many steps can be taken in a turn is set in the game rules. Additonally, some statements will use an action, the number of actions available per turn is defined by the game rules.

| value | explaination | examples | steps |
| --- | --- | --- | --- |
| _x_ | reference to a register 'x' | counter | 1 |
| #_x_ | reference to a meta variable 'x' | #x | 1 |
| _i_ | interger value | 14, -1 | 1 |
| _d_ | cardinal direction | N, E, S, W | 1 |
| _a_ _binop_ _b_| binary operation | 1 + 2, x % 10 | a + b + 1 |
| _unop_ _a_ | unary operation | ~a | a + 1 |
| scan | get the distance to the closest trench in some direction, or 0 if there is none | b = scan N; | 1 |
| ? | Get a random positive integer | ? | 1 |
| [e<sub>0</sub>, ..., e<sub>n</sub>] | pick a random value from a list | [N,S]  [1,24] | e<sub>0</sub> + ... + e<sub>n</sub> + 1 | 

binary operators: +, -, *, /, %, >, >=, <, <=, !=, =, |, &

unary operators: ~ (not)

| meta variable | explaination |
| --- | --- |
| #x | players 'x' position |
| #y | players 'y' position |
| #bombs | players remaining bombs |
| #shots | players remaining shots |
| #board_x | board width |
| #board_y | board height |



| statement | explaination | examples | actions | steps |
| --- | --- | --- | --- | --- |
| if (_c_) _a_ else _b_ | conditional execution | if (c) { ... } else { ... }  | 0 | c + a + 1, or c + b + 2 |
| if (_c_) _a_ | conditional execution | if (c) { ... }  | 0 | c + a + 1, or c + 2 |
| {s<sub>0</sub> ... s<sub>n</sub>} | A contained series of statements. | { ... } | 0 | s<sub>0</sub> + ... + s<sub>n</sub> |
| repeat(_x_) _s_ | Compile a statement 'x' times | repeat (2) ... | 0 | x * s |
| _a_ = _e_ | Assign a value to a register. | a = a + 1; a += 1; | 0 | e + 2 |
| label | Marks a location in the directive | loop: | 0 | 0 |
| move _d_ | Move the player 1 space in some direction. | move [N,S]; | 0 | d + 1 |
| shoot _d_ | Shoot in some direction, from the players current position. Any player in that direction, not in a trench will die. | shoot S; | 1 | d + 1 |
| bomb _d_ _p_ | Throw a bomb in some direct, some number of spaces. It explodes after 1 round, killing any player hit, unless they are in a fortified trench. Fortified trenches become unfortifed, unfortified trenches are destroyed. | bomb N (2+1); | 1 | d + p + 1 |
| fortify | Fortify the trench here, if there is a trench. | fortify; | 1 | 2 |
| fortify _d_ | Fortify the trench in some direction, if there is a trench. | fortify N; | 1 | d + 1 |
| trench | Dig a trench here. | trench; | 1 | 2 |
| trench _d_ | Dig a trench in some direction. | trench W; trench; | 1 | d + 1 |
| wait | Use an action on nothing. | wait; | 1 | 1 |
| pass | End the current turn. | pass; | 0 | 1 |
| goto | Jump to a location in the directive. | goto loop; | 0 | 1 |

## Game file
The game file specifies the players and rules of a particular game. It consists of key value pairs, seperated by semi-colons, defining a set of values. Pairs can be left out, in which case a default will be used.

| key | explaination |
| --- | --- |
| bombs | the number of bombs a player can throw in a game |
| shots | the number of shots a player can fire in a game |
| actions | the number of actions a player can take per turn |
| steps | the number of steps a player can can perform per turn |
| mode | the number of round that must pass before players can change directive, 0 meaning never. Any negative number meaning before every turn. |
| player | the id, x and y position, and source file of a player |
| board_x | the board width |
| board_y | the board height |
| nuke | how many rounds must pass, between every space on the board being blow up. 0 meaning never. |

In the following, every value is set to its default, except the player:
```
bombs:10;
shots:100;
actions:1;
steps:1000;
mode:0;
nuke:0;

board_x:20;
board_y:20;

player:1,5,5,./player1.tr;
player:2,8,9,./player2.tr;

```
