/*
* Copyright (c) 2016-2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#ifndef NVBLASTTKCOMMON_H
#define NVBLASTTKCOMMON_H


#include "NvPreprocessor.h"
#include "NvBlastTkGUID.h"


// Macro to load a uint32_t (or larger) with four characters
#define NVBLASTTK_FOURCC(_a, _b, _c, _d)	( (uint32_t)(_a) | (uint32_t)(_b)<<8 | (uint32_t)(_c)<<16 | (uint32_t)(_d)<<24 )


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
	enum { ClassID = NVBLASTTK_FOURCC(_id0, _id1, _id2, _id3) }


// Macro to declare standard object interfaces, enums, etc (serializable version)
#define NVBLASTTK_IMPL_DEFINE_SERIALIZABLE(_id0, _id1, _id2, _id3)										\
	NVBLASTTK_IMPL_DEFINE_IDENTIFIABLE(_id0, _id1, _id2, _id3);											\
																										\
	/* Begin TkSerializable */																			\
	virtual bool			serialize(physx::general_PxIOStream2::PxFileBuf& stream) const override;	\
	/* End TkSerializable */																			\
																										\
	/* Static deserialization function, called by TkFrameworkImpl::deserialize after header data */		\
	static TkSerializable*	deserialize(physx::general_PxIOStream2::PxFileBuf& stream, const NvBlastID& id)


// Macro to define class type data
#define NVBLASTTK_DEFINE_TYPE_IDENTIFIABLE(_name) \
	TkTypeImpl Tk##_name##Type::s_type("Tk" #_name, Tk##_name##Impl::ClassID, 0, nullptr)


// Macro to define class type data (serializable version)
#define NVBLASTTK_DEFINE_TYPE_SERIALIZABLE(_name) \
	TkTypeImpl Tk##_name##Type::s_type("Tk" #_name, Tk##_name##Impl::ClassID, Tk##_name##Impl::Version::Current, Tk##_name##Impl::deserialize)


#endif // ifndef NVBLASTTKCOMMON_H
