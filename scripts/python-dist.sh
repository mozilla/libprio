#!/usr/bin/env bash
# Build binary wheels of prio and upload them to pypi.
#
# INSTALL_PYTHON_ENV: install the corresponding python versions via pyenv
# SKIP_UPLOAD: if set, do not run twine for uploading packages
#
# https://twine.readthedocs.io/en/latest/#environment-variables
# TWINE_USERNAME: e.g. __token__
# TWINE_PASSWORD: Set this to the api token
# TWINE_REPOSITORY: e.g. pypi or testpypi
# TWINE_NON_INTERACTIVE: e.g. true

set -e

VERSIONS="
    3.6.0 
    3.7.0 
    3.8.0
"
INSTALL_PYTHON_ENV=${INSTALL_PYTHON_ENV:-false}
SKIP_UPLOAD=${SKIP_UPLOAD:-false}
TWINE_NON_INTERACTIVE=${TWINE_NON_INTERACTIVE:-true}

function ensure_environment {
    # check pyenv exists and enable the shell
    pyenv --version
    eval "$(pyenv init -)"

    if [[ $INSTALL_PYTHON_ENV != "false" ]]; then
        for version in $VERSIONS; do
            pyenv install $version --skip-existing
        done
    fi
    local error=0
    for version in $VERSIONS; do
        if ! pyenv shell "$version"; then
            error=$((error + 1))
        fi
    done
    pyenv shell system
    if ((error > 0)); then
        echo "missing installation of python versions"
    fi
}

function build_wheel {
    local version=$1
    pyenv shell "$version"
    pip install pytest scons
    make install
    python -m pytest tests -v
    make dist
}

cd "$(dirname "$0")/../python"

ensure_environment
make clean

for version in $VERSIONS; do
    build_wheel "$version"
done
ls dist

if [[ $SKIP_UPLOAD == "false" ]]; then
    pyenv shell "3.8.0"
    pip install twine
    twine upload --skip-existing "dist/*"
fi