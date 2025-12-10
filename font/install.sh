[[ $PWD != "font" ]]; cd font 

cp ./Trench.ttf ~/.local/share/fonts

[[ $PWD == "font" ]]; cd ..

fc-cache -f