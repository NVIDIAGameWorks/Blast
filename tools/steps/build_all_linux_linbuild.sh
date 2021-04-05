#!/bin/bash +x


# Set the blast root to the current directory
SCRIPT=$(readlink -f "$0")
export BLAST_ROOT_DIR=$(dirname $(dirname $(dirname $SCRIPT)))
echo $BLAST_ROOT_DIR

# Retrieve at least linbuild
echo "Getting build host deps for Linux ..."
$BLAST_ROOT_DIR"/tools/packman5/packman" pull $BLAST_ROOT_DIR"/deps/host-deps.packman.xml" --platform "linux-$(arch)"

pushd $BLAST_ROOT_DIR

_build/host-deps/linbuild/linbuild.sh ./generate_projects_linux.sh

_build/host-deps/linbuild/linbuild.sh tools/steps/build_all_linux.sh

popd
