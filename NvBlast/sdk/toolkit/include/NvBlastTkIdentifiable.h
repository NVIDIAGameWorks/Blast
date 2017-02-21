/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKIDENTIFIABLE_H
