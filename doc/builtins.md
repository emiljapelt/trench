[Back to overview](../README.md)

The trench language contains builtin functions, some of which relate to themes and features. They are all functions which can be called like any normal function in the language, but they are not variables. If a variable of the same name is declared, that variable will be used instead of the builting function, allowing user to overwrite them.

Some builtin functions return a result in the form of an `int`, which may be an error. The following table shows the different named errors, each of which has a corresponding builtin variable.

| Name | Value |
| --- | --- |
| _SUCCESS | 1 |
| _ERROR | 0 |
| _MISSING_RESOURCE | -1 |
| _OUT_OF_BOUNDS | -2 |
| _INVALID_TARGET | -3 |
| _OUT_OF_RANGE | -4 |
| _OBSTRUCTED | -5 |
| _MISSING_SPACE | -6 |

---

### Wait <sub><small>action</small></sub>
`wait` `int:()`

Does nothing, but spends an action.

**Returns:** 

Always `_SUCCESS`.

---

### Pass
`pass` `int:()`

Ends the current turn.

**Returns:**

Always `_SUCCESS`.

---

### Move <sub><small>action</small></sub>
`move` `int:(dir d)`

Attempt to move the player one field in direction `d`.

**Returns:**

| Value | Explaination |
| --- | --- |
| _SUCCESS | The player has moved |
| _OBSTRUCTED | The player did not move, either the currect or target field cotains an obstruction |
| _OUT_OF_BOUNDS | The target field is out of bounds |

---

### Read
`read` `int:()`

Every field can contain an `int` value, which is set using the `write` function. This function returns the value from the field the player is standing on.

**Features:** ipc

**Returns:**

The value writen on the players current field.

---

### Write
`write` `int:(int i)`

Writes `i` to the players current location

**Features:** ipc

**Returns:** 

Always `_SUCCESS`.


---

### Pager Read
`pager_read` `int:()`

Every player is equiped with a pager. It can the set a channel using `pager_set`, and messages are send on the channel using `pager_write`. 

This functions returns the oldest message, and removes it from the pager.

**Features:** ipc

**Returns:**

The newest message on the pager, or `0` if there are now messages to read.

---

### Pager Write
`pager_write` `int:(int i)`

Send `i` the pagers of all other players on the same channel.

**Features:** ipc

**Returns:**

`_SUCCESS` if at least one player received the message, otherwise `_ERROR`.

---

### Pager Set
`pager_set` `int:(int i)`

Changes the channel of the players pager to `i`.

**Features:** ipc

**Returns:**

`_SUCCESS` if the channel was changed, otherwise `_ERROR`.

---

### Scan
`scan` `field:(dir d, int i)`

Returns a snapshot of the properties of the field in direction `d`, at a distance of `i` fields. It is possible to scan past an obstruction. By default the is no limit to the size of `i`. If `i` is less the `0`, it becomes `0`.

---

### Look
`look` `int:(dir d, property p)`

Get the distance to the nearest field in the direction `d`, which has the property `p`. By default the is no limit to the range.

**Returns:**

The distance to the first field with property `p`, or `_ERROR` if no such field was found.

---

### Collect <sub><small>action</small></sub>
`collect` `int:(dir d) | int:()`

Attempt to collect resources from the ajecent field in direction `d`, or the players current field if `d` is not provided. 

Resources can be collected from TREE, MINE_SHAFT and CLAY fields.

**Returns:** 

If the targeted field is out of bounds `_OUT_OF_BOUNDS` is returned, if a resource was collected `_SUCCESS` is returned, otherwise `_ERROR` is returned.

---

### Say
`say` `int:(int i)`

Print `i` to the feed.

**Returns:**

Always `_SUCCESS`.

---

### Trench <sub><small>action</small></sub>
`trench` `int:(dir d) | int:()`

Attemp to create a trench on the adjecent field in direction `d`, or on the players current field if `d` is not provided. 

**Returns:**  

| Value | Explaination |
| --- | --- |
| _SUCCESS | A trench was created |
| _OUT_OF_BOUNDS | The target field is out of bounds |
| _INVALID_TARGET | The trench could not be created |

---

### Fortify <sub><small>action</small></sub>
`fortify` `int:(dir d) | int:()`

Attemp to fortify the adjecent field in direction `d`, or the players current field if `d` is not provided. This costs `5` `wood` (`fortify.cost`).

TRENCH, WALL and MINE_SHAFT fields can be fortified, but only once.

**Returns:**

| Value | Explaination |
| --- | --- |
| _SUCCESS | A field was fortified |
| _OUT_OF_BOUNDS | The target field is out of bounds |
| _INVALID_TARGET | The field could not be fortified |
| _MISSING_RESOURCE | The player did not have enough resources |

---

### Shoot <sub><small>action</small></sub>
`shoot` `int:(dir d)`

Fire a bullet in direction `d`, spending `1` `ammo` to do so. The bullet will travel until it hits an obstruction or a player which is not in cover, with a maximum range of `6` (`shoot.range`). The bullet will only hit a single player on a field.

**Themes:** military, forestry

**Returns:**

`_MISSING_RESOURCE` if the player is out of ammot, otherwise `_SUCCESS`.

---

### Disarm <sub><small>action</small></sub>
`disarm` `ìnt:(dir d)`

Remove all physical events from the adjecent field in direction `d`.

**Themes:** military, forestry

**Returns:**

`_OUT_OF_BOUNDS` if the target field does not exist, otherwise `_SUCCESS`.

---

### Plant Tree <sub><small>action</small></sub>
`plant_tree` `int:(dir d)`

Plant a tree on the adjecent field in direction `d`, spending `1` `#sapling`. This creates a physical event on the target field, which will turn the target field into a TREE field after 3 rounds (`plant_tree.delay`), if the field is EMPTY at that point. 

**Themes:** forestry

**Returns:** 

| Value | Explaination |
| --- | --- |
| _SUCCESS | A tree was plantet |
| _OUT_OF_BOUNDS | The target field is out of bounds |
| _MISSING_RESOURCE | The player did not have enough saplings |

---

### Chop <sub><small>action</small></sub>
`chop` `int:(dir d)`

Swing an axe in direction `d`, reach the adjecent field. If a player is on the field they will be struck and killed. If the field is a TREE it will be destroyed, and the player gains `10` `#wood` (`chop.wood_gain`), and a 30% chance of gaining `1` `#sapling`.

**Themes:** forestry

**Returns:** 

Always `_SUCCESS`.

---

### Wall <sub><small>action</small></sub>
`wall` `int:(dir d)`

Build a WALL in one field direction `d`. This cost `10` `wood` (`wall.cost`), and requires that the target field is EMPTY.

**Returns:**

| Value | Explaination |
| --- | --- |
| _SUCCESS | A wall was built |
| _OUT_OF_BOUNDS | The target field is out of bounds |
| _MISSING_RESOURCE | The player did not have enough wood |
| _INVALID_TARGET | The target field was not EMPTY |

---

### Bridge <sub><small>action</small></sub>
`bridge` `int:(dir d)`

Build a BRIDGE in one field direction `d`. This cost `20` `wood` (`bridge.cost`), and requires that the target field is OCEAN.

**Returns:**

| Value | Explaination |
| --- | --- |
| _SUCCESS | A wall was built |
| _OUT_OF_BOUNDS | The target field is out of bounds |
| _MISSING_RESOURCE | The player did not have enough wood |
| _INVALID_TARGET | The target field was not OCEAN |

### Throw clay <sub><small>action</small></sub>
`throw_clay` `int:(dir d, int i)`

Throw a ball of clay in direction `d`, upto a maximal range of `i`. If `i` is greater than `3` (`throw_clay.range`), it will be reduced to this. This costs `1` `#clay` (`throw_clay.cost`).

If the clay hits an obstruction it will be damaged, if it hits a player that player will die. Otherwise, if the ball lands on an EMPTY field, that field is converted to a CLAY field, and if that field was already a CLAY field, a unit of clay will be added to the field.

**Themes:** pottery

**Returns:**

`_MISSING_RESOURCE` if the player does not have enough clay, otherwise `_SUCCESS`.

---

### Clay golem <sub><small>action</small></sub>
`clay_golem` `int:()`

Create a golem of clay.

**Themes:** pottery

**Features:** fork

**Returns:**

---

### Craft
`craft` `int:(resource r)`

**Returns:**

---

### Drop
`drop` `int:(int i, resource r)`

**Returns:**

---

### Take
`take` `int:(int i, resource r)`

**Returns:**

---

### Mine shaft <sub><small>action</small></sub>
`mine_shaft` `int:(dir d) | int:()`

**Returns:**

---








### Bomb <sub><small>action</small></sub>
**themes:** military

**syntax:** `bomb d i`

### Mine <sub><small>action</small></sub>
**themes:** military

**syntax:** `mine d`

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


[Back to overview](../README.md)