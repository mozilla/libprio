.PHONY: all test clean

all:
	cd .. && CCFLAGS='-fPIC' scons && cd python
	mkdir -p build
	mkdir -p libprio
	cp -r ../build ../include libprio
	python3 setup.py build

install: all
	python3 setup.py install

test:
	tox

clean:
	cd .. && scons -c && rm .sconsign.dblite && cd python
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
