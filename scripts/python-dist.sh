#!/usr/bin/env bash
# Build binary wheels of prio and upload them to pypi.
#
# INSTALL_PYENV_VERSION: install the corresponding python versions via pyenv
# SKIP_UPLOAD: if set, do not run twine for uploading packages
# SKIP_WHEEL: if set, do not upload wheels to pypi due to manylinux non-conformance
#
# https://twine.readthedocs.io/en/latest/#environment-variables
# TWINE_USERNAME: e.g. __token__
# TWINE_PASSWORD: Set this to the api token
# TWINE_REPOSITORY: e.g. pypi or testpypi
# TWINE_NON_INTERACTIVE: e.g. 0

set -e

export TWINE_NON_INTERACTIVE=${TWINE_NON_INTERACTIVE:-true}
SKIP_UPLOAD=${SKIP_UPLOAD:-false}
INSTALL_PYENV_VERSION=${INSTALL_PYENV_VERSION:-false}
PYENV_VERSIONS="
    3.6.0
    3.7.0
    3.8.0
"

function ensure_pyenv() {
    # check pyenv exists and enable the shell
    local version=$1
    if [[ $INSTALL_PYENV_VERSION != "false" ]]; then
        pyenv install "$version" --skip-existing
    fi
    if ! pyenv shell "$version"; then
        echo "missing installation of python versions"
        exit
    fi
    pyenv shell system
}

function build_dist() {
    pip install --upgrade setuptools scons pytest
    make install
    python -m pytest tests -v
    make dist
}

function manylinux_main {
    # assume that python3 and pip3 are already set up
    python3 --version
    pip3 --version
    for version in /opt/python/*; do
        # only >= 3.6 is supported due to f-strings
        if [[ $version == *35* ]]; then
            continue
        fi
        tmp_venv=$(mktemp -d)
        "$version/bin/python3" -m venv "$tmp_venv"
        source "$tmp_venv/bin/activate"
        build_dist
        deactivate
    done
    mkdir wheelhouse
    for wheel in dist/*.whl; do
        # Retag the binary wheel as manylinux1 for acceptance into pypi,
        # however this is not actually manylinux compatible. Note that
        # `auditwheel repair` will cause Prio_init due to NSS incompatibilities.
        # auditwheel repair "$wheel"
        cp "$wheel" wheelhouse/"$(basename "${wheel/-linux_/-manylinux1_}")"
    done;
}

function macos_main {
    pyenv --version
    eval "$(pyenv init -)"
    for version in $PYENV_VERSIONS; do
        ensure_pyenv $version
    done
    for version in $PYENV_VERSIONS; do
        pyenv shell "$version"
        build_dist "$version"
    done
    mkdir wheelhouse
    cp dist/*.whl wheelhouse/
    pyenv shell system
}

cd "$(dirname "$0")/../python"
make clean

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # assumes manylinux2014 docker image
    manylinux_main;
elif [[ "$OSTYPE" == "darwin"* ]]; then
    macos_main;
else
    echo "unsupported environment"
fi
ls dist

if [[ $SKIP_UPLOAD == "false" ]]; then
    pip install twine
    twine upload --skip-existing "wheelhouse/*"
fi
