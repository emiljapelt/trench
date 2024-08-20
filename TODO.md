- Language feature levels
    - A game rule sets the level, and the compiler must then check for adherence
    - 0 features are core features, that must exist (move, labels, trench, shoot, goto, ...)
    - 1 features are basic features, (repeat, if-else)
    - 2 features are memory features, (registers, assignment, lookup)
    - 3 features are advanced features, (loops, switch, functions?)

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

- Update README.md
