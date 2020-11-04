# - Try to find PxShared SDK
# - Sets PXSHAREDSDK_INCLUDE_DIRS 

include(FindPackageHandleStandardArgs)

# Always try explicit PATH variable first
find_path(PXSHAREDSDK_PATH include/foundation/Px.h
	PATHS
	$ENV{PM_pxshared_PATH}
)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(PXSHAREDSDK
	DEFAULT_MSG
	PXSHAREDSDK_PATH
)

if (PXSHAREDSDK_FOUND)
	# NOTE: This include list is way too long and reaches into too many internals.
	# Also may not be good enough for all users.
	SET(PXSHAREDSDK_INCLUDE_DIRS 
		${PXSHAREDSDK_PATH}/include
		${PXSHAREDSDK_PATH}/include/foundation
	)
endif()
