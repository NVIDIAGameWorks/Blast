#!/bin/bash +x

set -e

# get number of CPU cores
if [ -f /proc/cpuinfo ]; then
    CPUS=`grep processor /proc/cpuinfo | wc -l`
else
    CPUS=1
fi

# Stackoverflow suggests jobs count of (CPU cores + 1) as a respectively good number!
JOBS=`expr $CPUS + 1`

# run make for all configs
pushd "$(dirname "$0")/../../compiler/linux64-debug-gcc"
make -j$JOBS
popd

pushd "$(dirname "$0")/../../compiler/linux64-release-gcc"
make -j$JOBS
popd

pushd "$(dirname "$0")/../../compiler/linux64-checked-gcc"
make -j$JOBS
popd

pushd "$(dirname "$0")/../../compiler/linux64-profile-gcc"
make -j$JOBS
popd
