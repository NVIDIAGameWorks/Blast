/*
* Copyright (c) 2017, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#pragma once
#include "kj/io.h"
#include "capnp/serialize.h"
#include "generated/NvBlastExtSerializationLL.capn.h"
#include <vector>
#include "NvBlastExtInputStream.h"
#include "NvBlastExtOutputStream.h"

#if !defined(BLAST_LL_ALLOC)
#include "NvBlastExtAllocator.h"
#endif
#include "NvBlastExtGlobals.h"

namespace Nv
{
	namespace Blast
	{
		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		class ExtSerialization
		{
		public:
			static TAsset* deserialize(const unsigned char* input, uint32_t size);
			static TAsset* deserializeFromStream(std::istream &inputStream);

			static bool serializeIntoExistingBuffer(const TAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize);
			static bool serializeIntoNewBuffer(const TAsset *asset, unsigned char **outBuffer, uint32_t &outSize);
			static bool serializeIntoStream(const TAsset *asset, std::ostream &outputStream);

		private:

			static void serializeMessageIntoNewBuffer(capnp::MallocMessageBuilder& message, unsigned char ** outBuffer, uint32_t &outSize);

			// Specialized
			static bool serializeIntoBuilder(TSerializationBuilder& assetBuilder, const TAsset* asset);
			static bool serializeIntoMessage(capnp::MallocMessageBuilder& message, const TAsset* asset);
			static TAsset* deserializeFromStreamReader(capnp::InputStreamMessageReader &message);
		};

		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		TAsset* ExtSerialization<TAsset, TSerializationReader, TSerializationBuilder>::deserialize(const unsigned char* input, uint32_t size)
		{
			kj::ArrayPtr<const unsigned char> source(input, size);

			kj::ArrayInputStream inputStream(source);

			std::vector<uint64_t> scratch;
			scratch.resize(size);
			kj::ArrayPtr<capnp::word> scratchArray((capnp::word*) scratch.data(), size);

			capnp::InputStreamMessageReader message(inputStream, capnp::ReaderOptions(), scratchArray);

			return deserializeFromStreamReader(message);
		}

		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		TAsset* ExtSerialization<TAsset, TSerializationReader, TSerializationBuilder>::deserializeFromStream(std::istream &inputStream)
		{
			Nv::Blast::ExtInputStream readStream(inputStream);

			capnp::InputStreamMessageReader message(readStream);

			return deserializeFromStreamReader(message);
		}

		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		bool ExtSerialization<TAsset, TSerializationReader, TSerializationBuilder>::serializeIntoExistingBuffer(const TAsset *asset, unsigned char *buffer, uint32_t maxSize, uint32_t &usedSize)
		{
			capnp::MallocMessageBuilder message;

			bool result = serializeIntoMessage(message, asset);

			if (result == false)
			{
				usedSize = 0;
				return false;
			}

			uint32_t messageSize = computeSerializedSizeInWords(message) * sizeof(uint64_t);

			if (maxSize < messageSize)
			{
				NvBlastLog logFn = gLog;

#if !defined(BLAST_LL_ALLOC)
				logFn = NvBlastTkFrameworkGet()->getLogFn();
#endif

				NVBLAST_LOG_ERROR(logFn, "When attempting to serialize into an existing buffer, the provided buffer was too small.");
				usedSize = 0;
				return false;
			}

			kj::ArrayPtr<unsigned char> outputBuffer(buffer, maxSize);
			kj::ArrayOutputStream outputStream(outputBuffer);

			capnp::writeMessage(outputStream, message);

			usedSize = messageSize;;
			return true;
		}

		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		bool ExtSerialization<TAsset, TSerializationReader, TSerializationBuilder>::serializeIntoNewBuffer(const TAsset *asset, unsigned char **outBuffer, uint32_t &outSize)
		{
			capnp::MallocMessageBuilder message;

			bool result = serializeIntoMessage(message, asset);

			if (result == false)
			{
				*outBuffer = nullptr;
				outSize = 0;
				return false;
			}

			serializeMessageIntoNewBuffer(message, outBuffer, outSize);

			return true;
		}

		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		bool ExtSerialization<TAsset, TSerializationReader, TSerializationBuilder>::serializeIntoStream(const TAsset *asset, std::ostream &outputStream)
		{
			capnp::MallocMessageBuilder message;

			bool result = serializeIntoMessage(message, asset);

			if (result == false)
			{
				return false;
			}

			Nv::Blast::ExtOutputStream blastOutputStream(outputStream);

			writeMessage(blastOutputStream, message);

			return true;
		}

		template<typename TAsset, typename TSerializationReader, typename TSerializationBuilder>
		void ExtSerialization<TAsset, TSerializationReader, TSerializationBuilder>::serializeMessageIntoNewBuffer(capnp::MallocMessageBuilder& message, unsigned char ** outBuffer, uint32_t &outSize)
		{
			uint32_t messageSize = computeSerializedSizeInWords(message) * sizeof(uint64_t);

			NvBlastExtAlloc allocFn = gAlloc;

#if !defined(BLAST_LL_ALLOC)
			allocFn = ExtAllocator::alignedAlloc16;
#endif

			unsigned char* buffer = static_cast<unsigned char *>(allocFn(messageSize));

			kj::ArrayPtr<unsigned char> outputBuffer(buffer, messageSize);
			kj::ArrayOutputStream outputStream(outputBuffer);

			capnp::writeMessage(outputStream, message);

			*outBuffer = buffer;
			outSize = messageSize;
		}
	}
}
