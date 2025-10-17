[Back to overview](../README.md)

The trench language contains builtin functions, some of which relate to themes and features. They are all functions which can be called like any normal function in the language, but they are not variables. If a variable of the same name is declared, that variable will be used instead of the builting function, allowing user to overwrite them.




### Wait <sub><small>action</small></sub>
**name:** `wait`

**signature:** `int:()`

Does nothing and returns `1`.

---

### Pass
**name:** `pass`

**signature:** `int:()`

Ends the current turn and returns `1`.

---

### Move <sub><small>action</small></sub>
**name:** `move`

**signature:** `int:(dir d)`


Attempt to move the player one field in direction `d`, returning `1` if successful, otherwise returning `0`. The movement could for example fail if there is an obstruction in direction `d`.

---

### Read
**name:** `read`

**signature:** `int:()`

**features:** ipc

Every field can contain an *int* value, which is set using the `write` function. `read` returns the value from the field the player is standing on.

---

### Write
**name:** `write`

**signature:** `int:(int i)`

**features:** ipc

Writes `ì` to the players current location and returns `1`;

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

Sends `i` the pagers of all other players, if the are on the same channel. Returns `1` if at least one player received the message, otherwise returns `0`.

---

### Pager Set
**name:** `pager_set`

**signature:** `int:(int i)`

**features:** ipc

Changes the channels of the players pager to `í`. Returns `1` if the channel was changed, otherwise returns `0`.

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
**syntax:** `collect d`

### Say
**syntax:** `say i`





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