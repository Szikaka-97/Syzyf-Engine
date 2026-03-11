#!/bin/bash

echo PREPARING SCRIPTS

if [ ! -d venv ]; then
	python3 -m venv venv
fi

source venv/bin/activate

python -m pip show libclang > /dev/null

if [ $? == 1 ]; then
	python -m pip install libclang
fi

python SerializationDatabase.py