echo "Building Trench..."

echo "Building engine..."
cd ./engine
source ./compile.sh
cd ..
mv -f ./engine/trench .

echo "Building compiler..."
cd ./compiler
dune build
cd ..
if [ -e trenchc ] 
then rm -f ./trenchc
fi
mv -f ./compiler/_build/default/src/trenchc.exe ./trenchc

echo "Done"
