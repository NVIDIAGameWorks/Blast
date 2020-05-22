#!/bin/bash +x

set -e
pushd "$(dirname "$0")/../.."
./generate_projects_linux.sh
popd

