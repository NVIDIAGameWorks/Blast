#!/bin/sh

# Command file for Doxygen build

# %1 must be the name of the doxyfile (w/o extension) and folder to create (e.g. blast_api)
if [ "$#" -lt 1 ]; then
  echo "***SCRIPTERROR: The first argument must be the name of the doxyfile (without extension)"
  exit 1
fi

set -e

NAME=$1
EXT="_docs"

DOCS_PATH=../$NAME$EXT

if [ -d $DOCS_PATH ]; then
  rm -rf $DOCS_PATH
fi

DOXYGEN_VERSION=1.8.13-linux-x86_64
echo "Using packman to get doxygen: $DOXYGEN_VERSION" 
$(dirname "$0")/../../buildtools/packman/linux/packman $PM_OPTIONS_EXT install doxygen $DOXYGEN_VERSION

DOXYGEN=$PM_PACKAGES_ROOT/doxygen/$DOXYGEN_VERSION/bin/doxygen

# run doxygen
$DOXYGEN $NAME.doxyfile
if [ $? -ne 0 ]; then
  echo "***SCRIPTERROR: doxygen build error"
  exit 1
fi

#unalias cp 2>/dev/null

# copy logo
cp blast_logo.png $DOCS_PATH/files

# copy index.html
cp index.html $DOCS_PATH
#getfacl index.html | setfacl -f - $DOCS_PATH/index.html

echo "***doxygen.sh succeeded"
exit 0
