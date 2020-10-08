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
// Copyright (c) 2016-2020 NVIDIA Corporation. All rights reserved.


#ifndef NVBLASTTKCOMMON_H
#define NVBLASTTKCOMMON_H


#include "NvBlastGlobals.h"
#include "NvBlastTkGUID.h"


// Macro to define standard object classes.  An intermediate class is defined which holds common implementations.
#define NVBLASTTK_IMPL_DECLARE(_name)												\
class Tk##_name##Type : public Tk##_name											\
{																					\
public:																				\
	/* Blank constructor generates a new NvBlastID and informs framework  */		\
	Tk##_name##Type()																\
	{																				\
		memset(&m_ID, 0, sizeof(NvBlastID));										\
		setID(TkGenerateGUID(this));												\
		TkFrameworkImpl::get()->onCreate(*this);									\
	}																				\
																					\
	/* This constructor takes an existing NvBlastID and informs framework  */		\
	Tk##_name##Type(const NvBlastID& id)											\
	{																				\
		memset(&m_ID, 0, sizeof(NvBlastID));										\
		setID(id);																	\
		TkFrameworkImpl::get()->onCreate(*this);									\
	}																				\
																					\
	/* Destructor informs framework  */												\
	~Tk##_name##Type()	{ TkFrameworkImpl::get()->onDestroy(*this); }				\
																					\
	/* Begin TkIdentifiable */														\
	virtual void				setID(const NvBlastID& id) override					\
	{																				\
		/* Inform framework of ID change */											\
		TkFrameworkImpl::get()->onIDChange(*this, m_ID, id);						\
		m_ID = id;																	\
	}																				\
	virtual const NvBlastID&	getID() const override { return getIDInternal(); }	\
	virtual const TkType&		getType() const override { return s_type; }			\
	/* End TkIdentifiable */														\
																					\
	/* Begin public API */															\
																					\
	/* Inline method for internal access to NvBlastID */							\
	const  NvBlastID&			getIDInternal() const { return m_ID; }				\
																					\
	/* End public API */															\
																					\
	/* Static type information */													\
	static TkTypeImpl	s_type;														\
																					\
private:																			\
	NvBlastID			m_ID;	/* NvBlastID for a TkIdentifiable object */			\
};																					\
																					\
/* Derive object implementation from common implementation class above */			\
class Tk##_name##Impl final : public Tk##_name##Type


// Macro to declare standard object interfaces, enums, etc.
#define NVBLASTTK_IMPL_DEFINE_IDENTIFIABLE(_id0, _id1, _id2, _id3)		\
	/* Begin TkObject */												\
	virtual void		release() override;								\
	/* End TkObject */													\
																		\
	/* Enums */															\
																		\
	/* Generate a ClassID enum used to identify this TkIdentifiable. */	\
	enum { ClassID = NVBLAST_FOURCC(_id0, _id1, _id2, _id3) }


// Macro to define class type data
#define NVBLASTTK_DEFINE_TYPE_IDENTIFIABLE(_name) \
	TkTypeImpl Tk##_name##Type::s_type("Tk" #_name, Tk##_name##Impl::ClassID, 0)


#endif // ifndef NVBLASTTKCOMMON_H
