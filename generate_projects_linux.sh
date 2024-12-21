#!/bin/bash +x

# Set the blast root to the current directory
SCRIPT=$(readlink -f "$0")
export BLAST_ROOT_DIR=$(dirname $SCRIPT)
echo $BLAST_ROOT_DIR

# Run packman to ensure dependencies are present and run cmake generation script last
echo "Getting build platform dependencies for Linux ..."
source $BLAST_ROOT_DIR"/buildtools/packman/packman" pull $BLAST_ROOT_DIR"/buildtools/build_platform_deps.xml" --platform linux

echo "Getting target platform dependencies for Linux ..."
source $BLAST_ROOT_DIR"/buildtools/packman/packman" pull $BLAST_ROOT_DIR"/target_platform_deps.xml" --platform linux --postscript $BLAST_ROOT_DIR"/buildtools/cmake_projects_linux.sh"
