.PHONY: all test clean

all:
	cd .. && CCFLAGS='-fPIC' scons && cd python
	mkdir -p build
	mkdir -p libprio
	cp -r ../build ../include libprio
	python3 setup.py build

install: all
	pip3 install .

dist: all
	pip3 install wheel
	python3 setup.py bdist_wheel

format:
	clang-format -style=Mozilla -i swig/libprio_wrap.c
	black setup.py src tests

test:
	tox

clean:
	cd .. && scons -c && rm -f .sconsign.dblite && cd python
	find . \( \
		-name "*.pyc" \
		-o -name "*.so" \
		\) -delete
	find . \( \
		-name "__pycache__" \
		-o -name "*.egg-info" \
		-o -name "htmlcov" \
		\) -exec rm -r {} +
	rm -rf build
	rm -rf libprio
	rm -rf dist
	rm -rf wheelhouse
