echo "Building Trench..."

echo "Building compiler..."
cd ./compiler
dune build
cd ..
if [ -e trenchc ] 
then rm -f ./trenchc
fi
mv -f ./compiler/_build/default/src/trenchc.exe ./trenchc

echo "Building engine..."
cd ./engine
source ./build.sh
cd ..
mv -f ./engine/trench .

echo "Done"
