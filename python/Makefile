.PHONY: all test clean

all:
	cd .. && CCFLAGS='-fPIC' scons && cd python
	mkdir -p build
	swig -python -outdir prio -o libprio_wrap.c libprio.i
	python3 setup.py build_ext

test:
	tox

clean:
	cd .. && scons -c && cd python
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
