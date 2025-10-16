[Back to overview](../README.md)

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