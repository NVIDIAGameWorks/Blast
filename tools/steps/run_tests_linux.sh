#!/bin/bash +x

set -e

# run tests
pushd "$(dirname "$0")/../../bin/linux64-gcc"
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./BlastUnitTestsDEBUG.elf --gtest_output=xml:BlastUnitTestsDEBUG.xml
echo \#\#teamcity[importData type=\'gtest\' parseOutOfDate=\'true\' file=\'bin/linux64-gcc/BlastUnitTestsDEBUG.xml\']
./BlastUnitTests.elf --gtest_output=xml:BlastUnitTests.xml
echo \#\#teamcity[importData type=\'gtest\' parseOutOfDate=\'true\' file=\'bin/linux64-gcc/BlastUnitTests.xml\']
popd
