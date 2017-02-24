/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKOBJECT_H
#define NVBLASTTKOBJECT_H


namespace Nv
{
namespace Blast
{

/**
Base class for all objects in Tk.  All TkObjects are releasable.
*/
class TkObject
{
public:
	/**
	Constructor clears userData.
	*/
	TkObject() : userData(nullptr) {}

	// Object API

	/**
	Release this object and free associated memory.
	*/
	virtual void	release() = 0;

protected:
	/**
	Destructor is virtual and not public - use the release() method instead of explicitly deleting a TkObject
	*/
	virtual			~TkObject() {}

public:
	// Data

	/**
	Pointer field available to the user.
	*/
	void*	userData;
};

} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTTKOBJECT_H
