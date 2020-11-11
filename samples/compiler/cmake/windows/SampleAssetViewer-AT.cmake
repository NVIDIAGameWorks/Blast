# Copy the dlls from the deps

ADD_CUSTOM_COMMAND(TARGET SampleAssetViewer POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${PHYSXSDK_DLLS}
	${BL_EXE_OUTPUT_DIR}
)
