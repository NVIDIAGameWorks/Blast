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


#ifndef NVBLASTEXTAUTHORINGPATTERNGENERATOR_H
#define NVBLASTEXTAUTHORINGPATTERNGENERATOR_H

#include "NvBlastGlobals.h"

namespace Nv
{
	namespace Blast
	{
		typedef float (*RNG_CALLBACK)(void);

		struct PatternDescriptor
		{
			RNG_CALLBACK RNG = nullptr;

			uint32_t interiorMaterialId = 1000;
		};

		struct UniformPatternDesc : public PatternDescriptor
		{
			uint32_t cellsCount	= 2;

			float radiusMin		= 0.0f;
			float radiusMax		= 1.0f;
			float radiusDistr	= 1.0f;

			float debrisRadiusMult = 1.0f;
		};

		struct BeamPatternDesc : public PatternDescriptor
		{
			uint32_t cellsCount;

			float radiusMin;
			float radiusMax;
		};

		struct RegularRadialPatternDesc : public PatternDescriptor
		{
			float radiusMin = 0.0f;
			float radiusMax = 1.0f;
			
			uint32_t radialSteps = 3;
			uint32_t angularSteps = 8;

			float aperture = .0f;

			float angularNoiseAmplitude = 0.0f;
			
			float radialNoiseAmplitude = 0.0f;
			float radialNoiseFrequency = 0.0f;

			float debrisRadiusMult = 1.0f;
		};


		struct DamagePattern
		{
			/**
			Used to compute activated chunks.
			*/
			float activationRadius;
			float angle; // For cone shape activation
			enum ActivationDistanceType
			{
				Point = 0,
				Line,
				Cone
			};
			ActivationDistanceType activationType = Point;
			// ----------------------------------------------


			uint32_t cellsCount;
			class Mesh** cellsMeshes = nullptr;

			virtual void release() = 0;
		};

		class PatternGenerator
		{
		public:
			virtual DamagePattern* generateUniformPattern(const UniformPatternDesc* desc) = 0;
			virtual DamagePattern* generateBeamPattern(const BeamPatternDesc* desc) = 0;
			virtual DamagePattern* generateRegularRadialPattern(const RegularRadialPatternDesc* desc) = 0;


			virtual DamagePattern* generateVoronoiPattern(uint32_t pointCount, const NvcVec3* points, int32_t interiorMaterialId) = 0;
			virtual void release() = 0;
		};

		NVBLAST_API void savePatternToObj(DamagePattern* pattern);
		
	} // namespace Blast
} // namespace Nv


#endif // ifndef NVBLASTEXTAUTHORINGMESHCLEANER_H