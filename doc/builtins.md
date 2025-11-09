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
**name:** `wait`

**signature:** `int:()`

Does nothing and returns `_SUCCESS`.

---

### Pass
**name:** `pass`

**signature:** `int:()`

Ends the current turn and returns `_SUCCESS`.

---

### Move <sub><small>action</small></sub>
**name:** `move`

**signature:** `int:(dir d)`


Attempt to move the player one field in direction `d`.

**Return values:**

| Value | Explaination |
| --- | --- |
| _SUCCESS | The player has moved |
| _OBSTRUCTED | The player did not move, either the currect or target field cotains an obstruction |
| _OUT_OF_BOUNDS | The target field is out of bounds |


---

### Read
**name:** `read`

**signature:** `int:()`

**features:** ipc

Every field can contain an `int` value, which is set using the `write` function. This function returns the value from the field the player is standing on.

---

### Write
**name:** `write`

**signature:** `int:(int i)`

**features:** ipc

Writes `i` to the players current location and returns `_SUCCESS`;

---

### Pager Read
**name:** `pager_read`

**signature:** `int:()`

**features:** ipc

Every player is equiped with a pager. It can the set a channel using `pager_set`, and messages are send on the channel using `pager_write`. This functions returns the oldest message, and removes it from the pager.

If there are no new messages, this returns `0`.

---
### Pager Write
**name:** `pager_write`

**signature:** `int:(int i)`

**features:** ipc

Sends `i` the pagers of all other players, if the are on the same channel, and returns `_SUCCESS` if at least one player received the message, otherwise returns `_ERROR`.

---

### Pager Set
**name:** `pager_set`

**signature:** `int:(int i)`

**features:** ipc

Changes the channels of the players pager to `i`. Returns `_SUCCESS` if the channel was changed, otherwise returns `_ERROR`.

---

### Scan
**name:** `scan`

**signature:** `field:(dir d, int i)`

Returns a snapshot of the properties of the field in direction `d`, at a distance of `i` fields. It is possible to scan past an obstruction. By default the is no limit to the size of `i`. If `i` is less the `0`, it becomes `0`.

---


### Look
**name:** `look`

**signature:** `int:(dir d, prop p)`

Returns the distance to the nearest field in the direction `d`, which has the property `p`. Returns `0` if no such field is found. By default the is no limit to the range.



### Collect
**name:** `collect`

**signature:** `int:(dir d)`

Attempts to collect resources from the ajecent field in direction `d`. If the targeted field is out of bounds `_OUT_OF_BOUNDS` is returned, if a resource was collected `_SUCCESS` is returned, otherwise `_ERROR` is returned.

Resources can be collected from TREE, MINE_SHAFT and CLAY fields

**signature:** `int:()`

Attempts to collect resources from the field the player is standing on. 



### Say
**name:** `say`

**signature:** `int:(int i)`

Prints `i` to the feed and returns `_SUCCESS`.





---

### Trench <sub><small>action</small></sub>
**name:** `trench`

**signature:** `int:(dir d)`

Attemps to create a trench one field in direction `d`. Returns `1` if a trench is created.

---

### Trench <sub><small>action</small></sub>
**name:** `trench`

**signature:** `int:()`

Attemps to create a trench at the players current location. Returns `1` if a trench is created.

---

### Fortify <sub><small>action</small></sub>
**name:** `fortify`

**signature:** `int:(dir d)`

Attemps to fortify the field one space in direction `d`, spending `5` wood doing so. Returns `1` if something became fortified. Walls and trenched can be fortified.

---

### Fortify <sub><small>action</small></sub>
**name:** `fortify`

**signature:** `int:()`

Attemps to fortify the field the player is standing on, spending `5` wood doing so. Returns `1` if something became fortified. Walls and trenched can be fortified.

---









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



### Throw clay
**type:** int

**syntax:** `throw_clay` *dir* *int*

throw some clay

---

### Clay golem
**type:** int

**syntax:** `clay_golem`

Create a golem

---


[Back to overview](../README.md)