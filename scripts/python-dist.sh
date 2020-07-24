#!/usr/bin/env bash
# Build binary wheels of prio and upload them to pypi.
#
# INSTALL_PYTHON_ENV: install the corresponding python versions via pyenv
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
INSTALL_PYTHON_ENV=${INSTALL_PYTHON_ENV:-false}
SKIP_UPLOAD=${SKIP_UPLOAD:-false}
VERSIONS="
    3.6.0
    3.7.0
    3.8.0
"

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

function build_dist {
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
    build_dist "$version"
done
ls dist

if [[ $SKIP_UPLOAD == "false" ]]; then
    pyenv shell "3.8.0"
    pip install twine
    # NOTE:the wheel is not manylinux compatible.
    # > Binary wheel 'prio-1.0-cp36-cp36m-linux_x86_64.whl' has an unsupported platform tag 'linux_x86_64'.
    # TODO: lie about being manylinux1 compliant, and note this in the README
    twine upload --skip-existing "dist/*"
fi