#
# Build NvBlast Linux
#

SET(BLASTEXT_PLATFORM_COMMON_FILES
)

SET(BLASTEXT_PLATFORM_INCLUDES
)

SET(BLASTEXT_COMPILE_DEFS
	# Common to all configurations
	${BLAST_SLN_COMPILE_DEFS};
	
	$<$<CONFIG:debug>:${BLAST_SLN_DEBUG_COMPILE_DEFS}>
	$<$<CONFIG:checked>:${BLAST_SLN_CHECKED_COMPILE_DEFS}>
	$<$<CONFIG:profile>:${BLAST_SLN_PROFILE_COMPILE_DEFS}>
	$<$<CONFIG:release>:${BLAST_SLN_RELEASE_COMPILE_DEFS}>
)

SET(BLAST_EXT_SHARED_LIB_TYPE SHARED)

#Exceptions are needed by boost
#This option doesn't work on Clang
IF (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	SET(BLASTEXT_PLATFORM_COMPILE_OPTIONS "-Wno-maybe-uninitialized" "-fexceptions" "-Wno-unused-parameter")
	SET(VHACD_COMPILE_FLAGS "-Wno-type-limits -Wno-ignored-qualifiers -Wno-unused-result")
ELSE ()
	SET(BLASTEXT_PLATFORM_COMPILE_OPTIONS "-Wno-return-type-c-linkage" "-pedantic" "-fexceptions" "-Wno-unused-parameter")
	SET(VHACD_COMPILE_FLAGS "-Wno-logical-op-parentheses -Wno-ignored-qualifiers")
ENDIF()
