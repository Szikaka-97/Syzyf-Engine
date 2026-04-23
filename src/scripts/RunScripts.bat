@echo off

echo PREPARING SCRIPTS

if not exist "venv" (
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

python GenerateTypeInfo.py %cmake_source_dir% %cmake_include_dir% %cmake_binary_dir%

if not %ERRORLEVEL% == 0 (
	exit
)

python SerializationDatabase.py %cmake_source_dir% %cmake_include_dir% SerializationDecls %cmake_binary_dir%/../compile_commands.json