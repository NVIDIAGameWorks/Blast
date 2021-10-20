#!/bin/bash +x

set -e

# run tests
bin_path="$(dirname "$0")/../../../../_build/linux-x86_64/release/blast-sdk/bin"
pushd $bin_path
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./UnitTests --gtest_output=xml:UnitTests.xml
echo \#\#teamcity[importData type=\'gtest\' parseOutOfDate=\'true\' file=\'UnitTests.xml\']
popd
