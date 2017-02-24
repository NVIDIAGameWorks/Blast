/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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
