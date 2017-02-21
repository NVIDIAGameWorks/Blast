/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTEXTPXLISTENER_H
#define NVBLASTEXTPXLISTENER_H


namespace Nv
{
namespace Blast
{

// Forward declarations
class ExtPxFamily;
class ExtPxActor;


/**
Physics Listener Interface.

Actor create/destroy events listener.
*/
class ExtPxListener
{
public:
	/**
	Interface to be implemented by the user. Will be called when ExtPxFamily creates new actor.

	\param[in]	family	Corresponding ExtPxFamily with new actor.
	\param[in]	actor	The new actor.
	*/
	virtual void	onActorCreated(ExtPxFamily& family, ExtPxActor& actor) = 0;

	/**
	Interface to be implemented by the user. Will be called when ExtPxFamily destroy an actor.

	\param[in]	family	Corresponding ExtPxFamily.
	\param[in]	actor	The actor to be destroyed.
	*/
	virtual void	onActorDestroyed(ExtPxFamily& family, ExtPxActor& actor) = 0;
};


} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTPXLISTENER_H
