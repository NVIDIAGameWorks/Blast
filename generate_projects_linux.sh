#!/bin/sh +x

# Set the blast root to the current directory
SCRIPT=$(readlink -f "$0")
export BLAST_ROOT_DIR=$(dirname $SCRIPT)
echo $BLAST_ROOT_DIR

# Run packman to ensure dependencies are present and run cmake generation script afterwards
echo "Running packman in preparation for cmake ..."

$BLAST_ROOT_DIR"/buildtools/packman/packman" pull $BLAST_ROOT_DIR"/dependencies.xml" --platform linux --postscript $BLAST_ROOT_DIR"/buildtools/cmake_projects_linux.sh"
