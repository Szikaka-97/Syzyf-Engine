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

if [ ! -d $cmake_binary_dir/codegen ]; then
	mkdir $cmake_binary_dir/codegen
fi

python GenerateTypeDatabase.py $cmake_binary_dir $cmake_include_dir $cmake_binary_dir/../compile_commands.json

if [ ! $? == 0 ]; then
	exit
fi

python GenerateTypeInfos.py $cmake_binary_dir/codegen/type_database.json

if [ ! $? == 0 ]; then
	exit
fi

python GenerateSerializationDatabase.py $cmake_binary_dir/codegen/type_database.json

if [ ! $? == 0 ]; then
	exit
fi

python GenerateSketchyStuff.py $cmake_binary_dir/codegen/type_database.json
