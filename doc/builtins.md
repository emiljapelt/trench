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
`look` `int:(dir d, prop p)`

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
`fortify` `int:(dir d)`

Attemps to fortify the field one space in direction `d`, spending `5` wood doing so. Returns `1` if something became fortified. Walls and trenched can be fortified.

---

### Fortify <sub><small>action</small></sub>
`fortify` `int:()`

Attemps to fortify the field the player is standing on, spending `5` wood doing so. Returns `1` if something became fortified. Walls and trenched can be fortified.

---

### Shoot <sub><small>action</small></sub>
**themes:** military

**syntax:** `shoot d`

### Disarm <sub><small>action</small></sub>
**themes:** military, forestry

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

### Throw clay <sub><small>action</small></sub>
**type:** int

**syntax:** `throw_clay` *dir* *int*

throw some clay

---

### Clay golem <sub><small>action</small></sub>
**type:** int

**syntax:** `clay_golem`

Create a golem

---

### Craft
**type:** int

**syntax:** `clay_golem`

---

### Drop
**type:** int

**syntax:** `clay_golem`

---

### Take
**type:** int

**syntax:** `clay_golem`

---

### Mine shaft
**type:** int

**syntax:** `clay_golem`

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