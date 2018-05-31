# Copy the dlls from the deps

ADD_CUSTOM_COMMAND(TARGET BlastUnitTests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${PXSHAREDSDK_DLLS}
	${PHYSXSDK_DLLS}
	${NVTOOLSEXT_DLL}
	$<TARGET_FILE_DIR:BlastUnitTests>
	)
