# View
Currently the view of the game is simply the map. This limits the size of the map, to how many characters can be shown on screen.
Thats annoying. It would be much nicer if the world could be nearly any size, and then the engine just shows a section of it, and that section can then at runtime, using the arrow keys, be moved around. Any thing out-of-bound should be rendered as an empty and black character.

- Add to the game_state, a view size (set in .trg) and coordinate (0,0 initialy).
- Change rendering logic
- Figure out how to receive non-blocking input (ncurses, kbhit, ...)
- Change view coordinate, when receiveing arrow input

# Reduce reserved words
There are a lot of reserved words (), which limits variable naming. 

Requires that properties become typed values (T_Prop), maybe they are just like meta variables, but prefixed by '@'.
look N @ocean
prop search = @player;

Requires that call syntax is used both for instructions and functions.

Maybe these instructions, should all become functions, such that they can return information to the player?

Options:
- Some kind of program transformation, such that the absyn part of the instructions still exists, and call statements are swapped out at some point, if no variable have taken their name, in the current scope.

- If the instruction become functions-ish, a new part of the state could contain predefined functions, where the instructions then have something to decide not to use the Instr_Call, but the relevant instruction instead.



# Pottery theme
- Clay fields that spread
- Clay throwing
- Clay golems, that are not on your team


# Optimize
- None: 0
- Simple/Size: 1, Removes uneassesary program parts, reducing the resulting instruction list size
- Speed: 2, Replaces some program parts with equvilent parts, which may result in few instructions executed, but may increase instruction list size. Stuff like replacing boolean logic operators, with ternaries, possibly giving early "return".

Each level includes the prior.
Is defined in .trg files.


# Docker
Create a docker file for distribution. Shouldn't be to diffucult.