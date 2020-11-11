# Copy the dlls from the deps

ADD_CUSTOM_COMMAND(TARGET SampleBase POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different 
	${PHYSXSDK_DLLS} ${NVTOOLSEXT_DLL} ${SHADOW_LIB_DLL} ${HBAO_PLUS_DLL} ${D3DCOMPILER_DLL}
	$<$<CONFIG:debug>:${BL_EXE_OUTPUT_DIR}/debug>
	$<$<CONFIG:checked>:${BL_EXE_OUTPUT_DIR}/checked> 
	$<$<CONFIG:profile>:${BL_EXE_OUTPUT_DIR}/profile>
	$<$<CONFIG:release>:${BL_EXE_OUTPUT_DIR}/release>
)
