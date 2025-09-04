# Global scope
    - Missing stack frame upper bound checks for global_access and global_assign
        - Analyse stack return information to find the limit.
    - Declaration order is messing with stuff.
        - With a global var 'v', and a local var 'v', only the local variable is accessible even before it is declares...


# Optimize
- None: 0
- Simple/Size: 1, Removes uneassesary program parts, reducing the resulting instruction list size
- Speed: 2, Replaces some program parts with equvilent parts, which may result in few instructions executed, but may increase instruction list size. Stuff like replacing boolean logic operators, with ternaries, possibly giving early "return".

Each level includes the prior.
Is defined in .trg files.


# Docker
Create a docker file for distribution. Shouldn't be to diffucult.