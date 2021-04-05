#!/bin/bash +x

if [[ ! -z "$1" ]] && [[ "$1" == "linbuild" ]]
then
echo "bulding with linbuild"
./tools/steps/build_all_linux_linbuild.sh
else
./tools/steps/build_all_linux.sh
fi
