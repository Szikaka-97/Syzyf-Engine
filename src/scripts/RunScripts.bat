@echo on

echo PREPARING SCRIPTS

if not exist "venv" (
	python -m venv venv
)

CALL venv/Scripts/activate.bat

python -m pip show libclang

if ERRORLEVEL 1 (
	python -m pip install libclang
)

python SerializationDatabase.py