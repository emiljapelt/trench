[Back to overview](../README.md)

# The board
The board is the world in which the game takes place. It is 2D with 4 directions, and a fixed size. Each field has a type, with some properties, which describes how they behave.


Fields may have attached events which are triggered, either when a character enters or leaves said field.

## Field types

### EMPTY

**visual** : <code style="background-color: black;">&#183;</code>

Represents walkable ground with nothing interesting on it.

---

### TRENCH

**visual** : <code style="background-color: black;">&#9491;</code>

Represents a trench. It can be fortifed. Characters placed here are safe from projectiles, but will can be blown up along with the trench, unless it is fortifed. In which case it can survive, and protect characters, once.

Can only be build on an empty field.

---

### ICE_BLOCK

**visual** : <code style="background-color: aqua;">&#9491;</code>

Represents another field, which has been frozen solid. It obstructs player movements, as well as projectiles. It will melt, leaving behind the original field, if it is hit by fire. The original field is lost, if the field is destroyed.

---

### TREE

**visual** : <code style="background-color: black; color:green;">&#8607;</code>

Represents a tree, which obstruct player movement, as well a projectiles. Saplings can be collected from it, and it can be chopped down to collect wood. It can additionally be destoyed by using fire.

---

### OCEAN

**visual** : <code style="background-color: blue;">~</code>

Represents the ocean. Importantly, chacters cannot and will not attempt to swim.

---

### WALL

**visual** : <code style="background-color: black; color: saddlebrown;">&#9491;</code>

Represents a wall, which can be fortifed to survice being destoyed once. It obstructs player movements, as well as projectiles. Can additionally be destoyed by using fire.

Can only be build on an empty field.

---

### BRIDGE

**visual** : <code style="background-color: blue; color: saddlebrown;">&#9491;</code>

Represent a bridge over water. It can only be build on ocean fields. 

---

### MINE_SHAFT

**visual** : <code style="background-color: black; color: white;">&#2612;</code>

Represent a mine shaft. It can only be build on empty fields. 

---

# The map file
A Predefined map can be written in a .trm-file. Map files must contains a rectangle of the following symbols:


| trm | field |
| --- | --- | 
|.| EMPTY |
|+| TRENCH |
|~| OCEAN |
|T| TREE |
|w| WALL |

---

[Back to overview](../README.md)