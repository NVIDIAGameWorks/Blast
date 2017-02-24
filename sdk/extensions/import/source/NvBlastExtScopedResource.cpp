/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NvBlastExtScopedResource.h"

#include <ApexSDK.h>
#include <RenderMeshAsset.h>
#include <DestructibleAsset.h>
#include <ModuleDestructible.h>


namespace Nv
{
namespace Blast
{

namespace ApexImporter
{

void ApexReleaser::release(nvidia::apex::RenderMeshAssetAuthoring& a)
{
	if (mApex)
		mApex->releaseAssetAuthoring(a);
}


void ApexReleaser::release(nvidia::apex::DestructibleAssetAuthoring& a)
{
	if (mApex)
		mApex->releaseAssetAuthoring(a);
}


void ApexReleaser::release(nvidia::apex::ModuleDestructible& a)
{
	if (mApex)
		mApex->releaseModule(&a);
}

} // namespace ApexImporter

} // namespace Blast
} // namespace Nv
