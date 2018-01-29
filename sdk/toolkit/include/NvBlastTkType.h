// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2016-2018 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTTKTYPE_H
#define NVBLASTTKTYPE_H

#include "NvBlastTypes.h"



namespace Nv
{
namespace Blast
{

/**
Interface for static (class) type data.  This data is used for identification in streams,
class-specific object queries in TkFramework, etc.  Only classes derived from TkIdentifiable
use TkType data.
*/
class TkType
{
public:
	/**
	The class name.

	\return the class name.
	*/
	virtual const char*	getName() const = 0;

	/**
	The data format version for this class.  When deserializing, this version must match the
	current version.  If not, the user may convert the data format using the format conversion
	extension.

	\return the version number.
	*/
	virtual uint32_t	getVersion() const = 0;

	/**
	Test for equality.  This type is used in static (per-class) data, so types are equal exactly
	when their addresses are equal.

	\param[in]	type	The TkType to compare with this TkType.

	\return true if this type equals the input type, false otherwise.
	*/
	bool				operator == (const TkType& type) const
	{
		return &type == this;
	}
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKTYPE_H
