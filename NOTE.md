# Field visual priority
Assign a priority (render layer) to field_visual. Such that players can hide in trenches again...

# Refactor event system
- Consider naming of stuff
- Player start/end turn events (so fewer checks are needed)

# Optimize
- None: 0
- Simple/Size: 1, Removes uneassesary program parts, reducing the resulting instruction list size
- Speed: 2, Replaces some program parts with equvilent parts, which may result in few instructions executed, but may increase instruction list size. Stuff like replacing boolean logic operators, with ternaries, possibly giving early "return".

Each level includes the prior.
Is defined in .trg files.


# Builtin System Refactor... again

I would like to refactor the builtin system, such that the shared library which players use, is no longer hardcoded in the engine and then exposed via some structure. It limits flexibility, and means that the two systems needs to agree ón a bunch of stuff, whic his annoying to implement.

Instead I would like a system in which the .trg-file includes, either by reference to a file or written directly, the source code for a shared library written in the trench language, which is then loaded. This would mean that the shared library could change without recompilation, and has a single definition (basically...).

## Preparation

Refactor the .trg-file parser to be json-ish, just loading generic structures and values, and then load these. Would be a nice change, and make it easier to make changes in the future.

The engine instructions return a boolean, where true indicates that the game should be rendered. Only builtin function actually do this, so instructions could become void functions, and the call instruction can trigger a rendering, if it calls a builtin which returns true.

## Post -paration

The theme system can be removed, as library functions can just be removed from the library source.

Change the resource system, so that resources are loaded from the .trg-file. The hope is that the engine no longer needs to know which resources exist, because it no longer implementes the shared library.

Manage a default shared library, from which function can be taken.

## Questions

How are events going to work? Maybe a new type of event could be made, which points into a players program, and then executes it on a separat stack? There likely still needs to exist the old version, for stuff such as drowing in the ocean.

There is a change, in that the library takes up space in the players programs. Might just have to live with that. 

Players will each have an instance of each funcion, so it is not very shared...

## Current implementation idea

An entry in the .trg-file called 'library' contains trench source code. The engine contains a bunch of builtin functions (syscalls mayhaps?) interacting directly on the game state. Such interactions could be:
- getting the first player of a field
- destroying an entity
- using resources
- gaining resources
- spending actions
- triggering a rendering
- setting overlay information
- ...

The library is compiled, with these 'syscall' things, which are essentially exposes in the same way that builtins are currently, as global state. The resulting local state, is used as the players programs initial global state, and the resulting program is prepended.

The functions from the shared library can be made 'atomic' by a syscall which stop spending of steps, which then need to be reenabled at the end.

