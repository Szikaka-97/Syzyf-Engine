@echo off

echo PREPARING SCRIPTS

if not exist "venv/Scripts/activate.bat" (
	python -m venv venv
)

CALL venv/Scripts/activate.bat

python -m pip show libclang

if %ERRORLEVEL% == 1 (
	python -m pip install libclang
)

set cmake_source_dir=%1
set cmake_include_dir=%2
set cmake_binary_dir=%3

if not exist %cmake_binary_dir%/codegen (
	mkdir "%cmake_binary_dir%/codegen"
)

python GenerateTypeDatabase.py %cmake_binary_dir% %cmake_include_dir% %cmake_binary_dir%/../compile_commands.json

if not %ERRORLEVEL% == 0 (
	exit
)

python GenerateTypeInfos.py %cmake_binary_dir%/codegen/type_database.json

if not %ERRORLEVEL% == 0 (
	exit
)

python GenerateSerializationDatabase.py %cmake_binary_dir%/codegen/type_database.json
