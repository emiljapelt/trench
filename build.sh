echo "Building Trench..."

#echo "Building engine..."
#cd ./machine
#source ./compile.sh
#cd ..
#mv -f ./machine/seplin .

echo "Building compiler..."
cd ./compiler
dune build
cd ..
if [ -e trenchc ] 
then rm -f ./trenchc
fi
mv -f ./compiler/_build/default/src/trenchc.exe ./trenchc

echo "Done"
