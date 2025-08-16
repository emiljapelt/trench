# Stack safety
Might needs better stack limit checking...

# Optimize
- None: 0
- Simple/Size: 1, Removes uneassesary program parts, reducing the resulting instruction list size
- Speed: 2, Replaces some program parts with equvilent parts, which may result in few instructions executed, but may increase instruction list size. Stuff like replacing boolean logic operators, with ternaries, possibly giving early "return".

Each level includes the prior.
Is defined in .trg files.


# Docker
Create a docker file for distribution. Shouldn't be to diffucult.