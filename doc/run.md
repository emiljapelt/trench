[Back to overview](../README.md)

# Running the game

## Manual build

The git repo can be fetched, and with the right setup the `./build.sh` script will build all the projects, and place the `trench` and `trenchc` programs at the top level of the repo, which can then be used to play the game.

This is a bit involved, but it can work.

The build requires
- opam (with an ocaml compiler >= 5.4.1) 
- dune (install with opam)
- menhir (install with opam)
- and a varity of packages to make gcc happy...

## Docker container

It may be easier to use Docker and VSCode with the extension 'Dev Containers' installed. 

Once these are installed open a terminal and run:

`docker pull ghcr.io/emiljapelt/trench:latest && docker create -t -i ghcr.io/emiljapelt/trench:latest`

Then open VSCode, find the 'Remote Explorer' menu (somewhere on the left side on the window). Here you should find a container which you then attach to by right clicking and selecting an option.

In the top of the attached VSCode instance, click 'Terminal' -> 'New Terminal'. This is where you will run the game. To do so with an exmaple, write this in the terminal:

`trench examples/example.trg`

If everything has gone well, a game of trench will start, and a little red guy will be walking in cicles.

You are now ready to play around with trench! There are a handful of example maps available, along with a game and player file which are ready to be messed with. Good luck soldier.

---

[Back to overview](../README.md)