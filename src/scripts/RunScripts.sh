#!/bin/bash

echo PREPARING SCRIPTS



if [ ! -f venv/bin/activate ]; then
	python3 -m venv venv
fi

source venv/bin/activate

python -m pip show libclang > /dev/null

if [ $? == 1 ]; then
	python -m pip install libclang
fi
cmake_source_dir=$1
cmake_include_dir=$2
cmake_binary_dir=$3

python GenerateTypeDatabase.py $cmake_source_dir $cmake_include_dir $cmake_binary_dir

if [ ! $? == 0 ]; then
	exit
fi

python SerializationDatabase.py $cmake_source_dir $cmake_include_dir SerializationDecls $cmake_binary_dir/../compile_commands.json