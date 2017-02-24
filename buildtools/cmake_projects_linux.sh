#!/bin/bash +x

echo "Starting $(date)"

[ -z "$BLAST_ROOT_DIR" ] && echo "BLAST_ROOT_DIR not defined." && exit 1;
[ -z "$PM_CMakeModules_VERSION" ] && echo "PM_CMakeModules_VERSION not defined." && exit 1;
[ -z "$PM_cmake_VERSION" ] && echo "PM_cmake_VERSION not defined." && exit 1;
[ -z "$PM_PACKAGES_ROOT" ] && echo "PM_PACKAGES_ROOT not defined." && exit 1;

CMAKE=$PM_cmake_PATH/bin/cmake

echo "Cmake: $CMAKE"

set -e

# Common cmd line params
CMAKE_CMD_LINE_PARAMS="-DTARGET_BUILD_PLATFORM=linux -DBL_LIB_OUTPUT_DIR=$BLAST_ROOT_DIR/lib/linux64-gcc -DBL_DLL_OUTPUT_DIR=$BLAST_ROOT_DIR/bin/linux64-gcc -DBL_EXE_OUTPUT_DIR=$BLAST_ROOT_DIR/bin/linux64-gcc"

#configs=("debug" "profile" "checked" "release")
configs=("debug" "release")
for config in "${configs[@]}"
do
	# Generate  projects
	rm -r -f $BLAST_ROOT_DIR/compiler/linux64-$config-gcc/
	mkdir $BLAST_ROOT_DIR/compiler/linux64-$config-gcc/
	cd $BLAST_ROOT_DIR/compiler/linux64-$config-gcc/
	$CMAKE $BLAST_ROOT_DIR -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=$config $CMAKE_CMD_LINE_PARAMS
	cd $BLAST_ROOT_DIR
done
