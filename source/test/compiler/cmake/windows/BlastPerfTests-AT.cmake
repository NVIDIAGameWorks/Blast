# Copy the dlls from the deps

ADD_CUSTOM_COMMAND(TARGET BlastPerfTests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${PXSHAREDSDK_DLLS}
	$<TARGET_FILE_DIR:BlastPerfTests>
)
