echo "Building Trench..."

echo "Building compiler..."
cd ./compiler
if [ -d _build ]
then rm -rf ./_build
fi

dune build

cd ..

if [ -e trenchc ] 
then rm -f ./trenchc
fi
cp ./compiler/_build/default/src/trenchc.exe ./trenchc


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
