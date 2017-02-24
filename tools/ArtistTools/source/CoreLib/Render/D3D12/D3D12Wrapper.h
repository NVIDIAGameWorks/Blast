#pragma once

#include <d3d11.h>

namespace D3DWrapper
{
	inline void SetDebugName(ID3D11DeviceChild* pResource, const char *name)
	{
		if (pResource) pResource->SetPrivateData( WKPDID_D3DDebugObjectName, strlen(name), name);
	}
}

#define SET_D3D_DEBUG_NAME(x, name) D3DWrapper::SetDebugName(x, name);
#define SET_D3D_DEBUG_NAME_VAR(x) D3DWrapper::SetDebugName(x, #x);
