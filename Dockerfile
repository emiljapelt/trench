FROM ubuntu:26.04

SHELL ["/bin/bash", "-c"]
CMD ["sh", "-c", "cd /trench && exec bash"]

ARG BRANCH

RUN apt-get update && apt-get install -y libzstd-dev git opam &&\
    opam init --compiler=5.4.1 --disable-sandboxing --shell-setup -y &&\
    opam update &&\
    opam switch create trench ocaml-base-compiler.5.4.1 &&\
    opam switch set trench &&\
    opam install dune menhir -y &&\
    git clone https://github.com/emiljapelt/trench &&\
    cd trench &&\
    git checkout $BRANCH &&\
    eval $(opam env) &&\
    ./build.sh &&\
    cp ./trench /bin/ &&\
    cp ./trenchc /bin/ &&\
    mkdir /tmp/trench &&\
    cp -r ./maps ./examples /tmp/trench &&\
    cd .. &&\
    rm -rf /trench &&\
    mv /tmp/trench /

# Copy useful files to a "lab" folder in root, and delete the build folder

#Maybe not?
#ENTRYPOINT ["/bin/bash", "-c" , "cd /trench && eval $(opam env)"]

## branch as argument?