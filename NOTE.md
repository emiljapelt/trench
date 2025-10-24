# Field visual priority
Assign a priority (render layer) to field_visual. Such that players can hide in trenches again...

# Refactor event system
- Consider naming of stuff
- Player start/end turn events (so fewer checks are needed)

# Builtin variables
- Very similar system to builtin functions
- Removes the need for meta variables
- Cool, but is it better?

# Optimize
- None: 0
- Simple/Size: 1, Removes uneassesary program parts, reducing the resulting instruction list size
- Speed: 2, Replaces some program parts with equvilent parts, which may result in few instructions executed, but may increase instruction list size. Stuff like replacing boolean logic operators, with ternaries, possibly giving early "return".

Each level includes the prior.
Is defined in .trg files.


# Docker
Create a docker file for distribution. Shouldn't be to diffucult.