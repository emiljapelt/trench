- Language feature levels
    - A game rule sets the level, and the compiler must then check for adherence
    - FL0 are core features, that must exist (move, labels, trench, shoot, goto, ...)
    - FL1 are basic features, (repeat, if-else)
    - FL2 are memory features, (registers, assignment, lookup)
    - FL3 are advanced features, (loops, switch, functions?)

- Implement switch-statement
    - sugar for if-else
- Implement loops (while & for)
- Implement functions?

- Change random syntax
    - Random int, is still: '?'
    - Select random becomes: ?(...)  /  ?(N N E)  /  ?(1 (1+1) (1+2)) 

- Direction operators
    - dir + int, for clockwise. N + 1 = E
    - dir - int, for counter-clockwise. N - 1 = W

- Consider getting replacing registers, with stack variables
    - Declaration must be moved to the top of their scope
        - Assignment declarations are split, leaving the assignment inplace
    - Stack decrease instructions needed, p0 could be used for stack increase

- Update README.md
