echo "Building Trench..."

echo "Building compiler..."
cd ./compiler
dune build
cd ..
if [ -e trenchc ] 
then rm -f ./trenchc
fi
mv -f ./compiler/_build/default/src/trenchc.exe ./trenchc
rm -rf ./compiler/_build

echo "Building engine..."
cd ./engine
source ./build.sh
cd ..
mv -f ./engine/trench .

echo "Building cartographer..."
cd ./cartographer
source ./build.sh
cd ..

echo "Done"
