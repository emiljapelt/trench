```
╶┬╴                                 
 │ ┌╴ ┌─┐         ╷                 
 │ │  │ │         │                 
 │ ╵  ├─┘     ┌─╴ ├─┐               
 ╵    │   ╷   │   │ │               
      └─╴ ├─┐ └─╴ ╵ ╵               
          │ │                       
          ╵ ╵            
```

Trench is a game played by programming. Players write scripts, which describes how their character behaves during the game. A game file is then written, configuring settings, team setups, the map. This can then be executed, last team standing wins!

The language ([trench](doc/language.md)) used to describe player behavior, is split into themes and feature sets. These can be enabled in the game file, changing what players are allow to do.

The game is split into rounds, in which each character will take turns executing a part of their program. A limit on the number of instructions and actions that a character can execute on their turn is enforced to end a turn. 

Details:

[The trench langauge](doc/language.md)

[The board](doc/board.md)

[The game file](doc/game.md)
