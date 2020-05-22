#!/bin/bash +x

set -e

# run tests
pushd "$(dirname "$0")/../../bin/linux64-gcc/debug"
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./BlastUnitTests.elf --gtest_output=xml:BlastUnitTestsDEBUG.xml
echo \#\#teamcity[importData type=\'gtest\' parseOutOfDate=\'true\' file=\'bin/linux64-gcc/BlastUnitTestsDEBUG.xml\']
popd
