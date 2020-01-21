#!/bin/bash +x

pushd "$(dirname "$0")"
python "create_packages.py" $@
popd
