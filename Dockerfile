FROM ubuntu:24.04

SHELL ["/bin/bash", "-c"]

RUN apt-get update && apt-get install -y libzstd-dev git opam &&\
    opam init --compiler=5.4.1 --disable-sandboxing --shell-setup -y &&\
    opam update &&\
    opam switch create trench ocaml-base-compiler.5.4.1 &&\
    opam switch set trench &&\
    opam install dune menhir -y &&\
    git clone https://github.com/emiljapelt/trench &&\
    cd trench &&\
    eval $(opam env) &&\
    ./build.sh &&\
    cp ./trench /bin/ &&\
    cp ./trenchc /bin/

#Maybe not?
#ENTRYPOINT ["/bin/bash", "-c" , "cd /trench && eval $(opam env)"]
