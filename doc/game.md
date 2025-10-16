[Back to overview](../README.md)

# The Game file

Game files end with the .trg extension. These describe the setup for game, including rules, team setup, map definition etc. Everything in the game file is written as key-value pairs, with some value being more comples than others. It looks similar to JSON. semi-colons are optional and comments start with //.

These are the files feed to the game engine, to start a game.

Example:
```
mode: inf
seed: 1234 // The best seed...
map: 20,29

actions: 2;
steps: 50;

resources: {
    // Mana overload!
    mana: 300 of 100
    wood: 0
}

themes: *
features: ipc, memory, control, loops

team: {
    name: shovelknights;
    color: 50, 20, 128;
    origin: 8,8;
    player: {
        name: john
        file: john.tr
    };
    player: {
        name: jane
        origin: 1,1
        file: ./saved/jane/first.tr
    };
}

team: {
    name: best_player
    color: 255,0,0
    origin: 16,16
    player: { name: god; file: /god.tr; }
}
```

## Keys

### actions
The value must be a positive integer. It defines how many actions a character can execute per turn.

**Default**: 1

---

### steps
The value must be a positive integer. It defines how many instructions a character can execute per turn. 

**Default**: 100

---

### mode
The value must be an integer, "inf" or "manual". It defines how often players are allowed to change their characters programming.

| values | meaning |
| --- | --- |
| 0, inf | Changes never happen |
| -x, manual | Changes can happen before each players turn |
| x | Changes can happen every *x* rounds |

**Default**: 0

---

### map
The value must be either two positive integers, or a file path.

If two integers, *x* and *y* are given, an empty map of that size is used. If a map file is given, the described map is generated and used.


**Default**: 20, 20

---

### nuke
The value must be a non-negative integer. If the value is 0 nothing happens, otherwise with a value of *x*, everytime *x* round have passed every field on the board is hit by a bomb.

**Default**: 0

---

### seed
The value must be an integer. If set the given value will be used as the inital seed for the random number generator. Otherwise a random seed will be picked.


**Default**: _

---

### time_scale
The value must a positive float. The given value is used to scale how slow the game runs. 0.0 meaning as fast as possible, and 2.0 meaning twice as slow as normal.

**Default**: 1.0

---

### themes
The value must be a comma seperated list of theme names, or an asterisk. The asterisk meaning *all* themes.

The currentlt available themes are:
- military
- wizardry
- forestry

**Default**: _

---

### features
The value must be a comma seperated list of feature names, or an asterisk. The asterisk meaning *all* features.

The currently available features are:
- random
- memory
- ipc
- loops
- control
- sugar
  
**Default**: _

### resources
The value must be a collection of key-value pairs. The keys are the resource names as described by the enabled themes. a value must be some a positve integers in one of the following syntaxes:

| syntax | meaning |
| --- | --- |
| *x* | Characters start with *x* of that resource, and there is no upper limit |
| *x* of *y* | Characters start with *x* of that resource, and the upper limit is *y*. *x* can be larger that *y*, but characters cannot regain beyond *y*.

The themes require the following resources:

| theme | resources |
| --- | --- |
| military | bomb, shot |
| wizardry | mana |
| forestry | sapling |
| | wood |

---

### team
Descibes a single team. 

Team configurations:
| key | value |
| --- | --- |
| name | a word. |
| color | three, comma seperated, integers in the range 0 - 255 i.e. an RGB color. |
| origin | two, comma seperated, integers. Describes where the team spawns. |
| player | Multiple allowed. |

<br>

Player configurations:
| key | value |
| --- | --- |
| name | a word. |
| origin | two, comma seperated, integers. Offsets this players spawn point. |
| file | A path to the file which the character executes. |

---

[Back to overview](../README.md)