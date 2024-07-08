#!/bin/bash

if [[ $PWD != *"DepView" ]]
then
    echo "Use from main project dir!"
    exit 1
fi

if [ ! -d '3rdparty' ]
then
	mkdir 3rdparty
fi
cd 3rdparty

if [[ `sha256sum raylib-5.0_linux_amd64.tar.gz` != "5724b8d89c7cedd0c582d022f195169fb3fc27646dac376238da7a2df39aa59c"* ]]
then
    echo "Downloading raylib"
    wget https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz
fi
echo "Extracting raylib"
tar -xvf raylib-5.0_linux_amd64.tar.gz --transform='s#raylib-5.0_linux_amd64/##' raylib-5.0_linux_amd64/include raylib-5.0_linux_amd64/lib

stat 'include/raygui.h' &> /dev/null
if [[ 0 -ne $? ]]
then
	if [ ! -d 'repos' ]
	then 
		mkdir repos
	fi
	cd repos

	git clone --depth 1 --branch 4.0 https://github.com/raysan5/raygui.git
	cd ..
	ln -s ../repos/raygui/src/raygui.h include/raygui.h
	ln -s ../repos/raygui/examples/custom_file_dialog/gui_window_file_dialog.h include/gui_window_file_dialog.h
fi

# sha256sum -c graphviz-11.0.0.tar.gz.sha256
# if [[ 0 -ne $? ]]
# then
# 	echo "Downloading graphviz"
# 	wget https://gitlab.com/api/v4/projects/4207231/packages/generic/graphviz-releases/11.0.0/graphviz-11.0.0.tar.gz.sha256
# 	wget https://gitlab.com/api/v4/projects/4207231/packages/generic/graphviz-releases/11.0.0/graphviz-11.0.0.tar.gz
# 	sha256sum -c graphviz-11.0.0.tar.gz.sha256
# 	if [[ 0 -ne $? ]]
# 	then
# 		echo "Given file is corrupted"
# 		exit 1
# 	fi 
# fi
# echo "Extracting graphviz"
# tar -xvf graphviz-11.0.0.tar.gz
