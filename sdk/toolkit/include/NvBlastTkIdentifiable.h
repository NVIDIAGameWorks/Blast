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


#ifndef NVBLASTTKIDENTIFIABLE_H
#define NVBLASTTKIDENTIFIABLE_H


#include "NvBlastTkObject.h"

#include "NvBlastTypes.h"


namespace Nv
{
namespace Blast
{

// Forward declarations
class TkType;


/**
TkIdentifiable objects have getID and setID methods for individual objects.  They also have a type (class) identification.
*/
class TkIdentifiable : public TkObject
{
public:
	// Identifiable API

	/**
	Return the ID associated with this object.

	\return the ID for this object.
	*/
	virtual const NvBlastID&	getID() const = 0;

	/**
	Set the ID for this object.
	*/
	virtual void				setID(const NvBlastID& id) = 0;

	/**
	Access to the static (class) type data for this object.

	\return the static type data for this object type.
	*/
	virtual const TkType&		getType() const = 0;

	/**
	Integer field available to the user which may be serialized.
	*/
	uint64_t	userIntData;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKIDENTIFIABLE_H
