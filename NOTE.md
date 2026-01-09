# Consolidation of entity
Entities can be destroyed, but they are handled very differently. 

Larger refactor

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


# Docker
Create a docker file for distribution. Shouldn't be to diffucult?







ToDo
- [X] Boats should be able to sail under bridges, i think
- [X] Make players drop their items on death, and also remove and free them
- [X] Make round a builtin variable
- [X] Make players enter their intial field
- [X] Make game start in a paused state
- [X] Fix crash when dismounting nothing
- [-] Fix memory leak (and malloc abuse) in movement functions such as 'move_player_to_location', caused by use of entity.of_...
- [ ] Start working on test game and real game. (smallish maps see note in F2??)
- [ ] MORE TESTING!

