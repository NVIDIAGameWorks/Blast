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

//! @file
//!
//! @brief Defines the SpatialAccelerator API used by the BooleanTool

#ifndef NVBLASTEXTAUTHORINGACCELERATOR_H
#define NVBLASTEXTAUTHORINGACCELERATOR_H

#include "NvBlastExtAuthoringTypes.h"


namespace Nv
{
    namespace Blast
    {

        class Mesh;

        /**
            Acceleration structure interface.
        */
        class SpatialAccelerator
        {
        public:

            /**
                Set state of accelerator to return all facets which possibly can intersect given facet bound.
                \param[in] pos Vertex buffer
                \param[in] ed Edge buffer
                \param[in] fc Facet which should be tested.
            */
            virtual void    setState(const NvcBounds3* bounds) = 0;

            /**
                Set state of accelerator to return all facets which possibly can intersect given facet.
                \param[in] pos Vertex buffer
                \param[in] ed Edge buffer
                \param[in] fc Facet which should be tested.
            */
            virtual void    setState(const Vertex* pos, const Edge* ed, const Facet& fc) = 0;
            /**
                Set state of accelerator to return all facets which possibly can cover given point. Needed for testing whether point is inside mesh.
                \param[in] point Point which should be tested.
            */
            virtual void    setState(const NvcVec3& point) = 0;
            /**
                Recieve next facet for setted state.
                \return Next facet index, or -1 if no facets left.
            */
            virtual int32_t getNextFacet() = 0;


            virtual void setPointCmpDirection(int32_t dir) = 0;
            
            virtual void release() = 0;

            virtual ~SpatialAccelerator() {}
        };

        /**
            Used for some implementations of spatial accelerators.
        */
        class SpatialGrid
        {
        public:
            virtual void setMesh(const Nv::Blast::Mesh* m) = 0;

            virtual void release() = 0;
        };
    } // namespace Blast
} // namsepace Nv


#endif // ifndef NVBLASTEXTAUTHORINGACCELERATOR_H
