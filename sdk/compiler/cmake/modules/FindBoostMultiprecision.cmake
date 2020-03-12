# - Try to find tinyObjLoader
# Once done this will define
#  BOOSTMULTIPRECISION_FOUND - System has tinyObjLoader
#  BOOSTMULTIPRECISION_INCLUDE_DIRS - The tinyObjLoader include directories

INCLUDE(FindPackageHandleStandardArgs)

#TODO: Proper version support
FIND_PATH(		BOOSTMULTIPRECISION_PATH boost/multiprecision
				PATHS 
				$ENV{PM_BoostMultiprecision_PATH}
				${GW_DEPS_ROOT}/BoostMultiprecision/${BoostMultiprecision_FIND_VERSION}
				NO_DEFAULT_PATH
				)

MESSAGE("Boost Multiprecision Loader: " ${BOOSTMULTIPRECISION_PATH})
	
SET(BOOSTMULTIPRECISION_INCLUDE_DIRS
	${BOOSTMULTIPRECISION_PATH}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(BoostMultiprecision DEFAULT_MSG BOOSTMULTIPRECISION_INCLUDE_DIRS)

mark_as_advanced(BOOSTMULTIPRECISION_INCLUDE_DIRS)