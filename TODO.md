
- Implement functions?

- Direction operators
    - Fix subtraction/counter-clockwise

- Consider new inter directive comms (current: global arrays)
    - Private arrays
    - Field data

- Work on stablizing synchronous round execution
    - Should nukes be part of the attack phase, or the final part of a round

- Consider how bombing works
    - Currently bombs are explode immediatly, because the bomb chain was error prone
    - But maybe the delay from the bomb chain is good for balance
        - If so, a new implementation is needed

- Refactor visuals of instructions
    - Some are a bit hasty
    - Maybe dont clear visuals immediatly
        - Instead leave that to the round code, such that async round phases can look like everything happens at once
        - Maybe a seperate matrix for visuals makes sense
            - It would make clearing the visuals quick, by using 'memset'