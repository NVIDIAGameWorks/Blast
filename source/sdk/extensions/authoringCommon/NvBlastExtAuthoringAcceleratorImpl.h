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


#ifndef NVBLASTEXTAUTHORINGACCELERATORIMPL_H
#define NVBLASTEXTAUTHORINGACCELERATORIMPL_H

#include <set>
#include <vector>
#include "NvBlastExtAuthoringAccelerator.h"

namespace Nv
{
    namespace Blast
    {

        class Mesh;

        /**
            Dummy accelerator iterates through all facets of mesh.
        */
        class DummyAccelerator : public SpatialAccelerator
        {
        public:
            /**
                \param[in] count Mesh facets count for which accelerator should be built.
            */
            DummyAccelerator(int32_t count);
            virtual void    release() override;

            virtual void    setState(const NvcBounds3* bounds) override;
            virtual void    setState(const Vertex* pos, const Edge* ed, const Facet& fc) override;
            virtual void    setState(const NvcVec3& point) override;
            virtual int32_t getNextFacet() override;

            virtual void    setPointCmpDirection(int32_t dir) override { NV_UNUSED(dir); }

        private:
            int32_t m_count;
            int32_t m_current;
        };

        struct SegmentToIndex
        {
            float coord;
            uint32_t index;
            bool end;

            SegmentToIndex(float c, uint32_t i, bool end) : coord(c), index(i), end(end) {}

            bool operator<(const SegmentToIndex& in) const
            {
                if (coord < in.coord) return true;
                if (coord > in.coord) return false;
                return end < in.end;
            }
        };



        class Grid : public SpatialGrid
        {
        public:
            friend class GridAccelerator;

            Grid(int32_t resolution);
            virtual void release() override;

            virtual void setMesh(const Nv::Blast::Mesh* m) override;

        private:
            int32_t m_resolution;
            int32_t m_r3;
            int32_t m_mappedFacetCount;
            NvcVec3 m_spos;
            NvcVec3 m_deltas;
            std::vector< std::vector<int32_t> > m_spatialMap;
        };

        class GridAccelerator : public SpatialAccelerator // Iterator to traverse the grid
        {
        public:
            GridAccelerator(Grid* grd);
            virtual void    release() override;

            virtual void    setState(const NvcBounds3* bounds) override;
            virtual void    setState(const Vertex* pos, const Edge* ed, const Facet& fc) override;
            virtual void    setState(const NvcVec3& point) override;
            virtual int32_t getNextFacet() override;
            virtual void    setPointCmpDirection(int32_t dir) override;

        private:
            Grid* m_grid;

            // Iterator data
            std::vector<uint32_t> m_alreadyGotFlag;
            uint32_t m_alreadyGotValue;
            std::vector<int32_t> m_cellList;
            int32_t m_gotCells;
            int32_t m_iteratorCell;
            int32_t m_iteratorFacet;
            int32_t m_pointCmdDir;
        };


        class SweepingAccelerator : public SpatialAccelerator
        {
        public:
            /**
            \param[in] count Mesh facets count for which accelerator should be built.
            */
            SweepingAccelerator(const Nv::Blast::Mesh* in);
            virtual void     release() override;

            virtual void    setState(const Vertex* pos, const Edge* ed, const Facet& fc) override;
            virtual void    setState(const NvcBounds3* bounds) override;
            virtual void    setState(const NvcVec3& point) override;
            virtual int32_t getNextFacet() override;
            virtual void    setPointCmpDirection(int32_t dir) override { NV_UNUSED(dir); }

        private:


            /*
                For fast point test.
            */
            std::vector<std::vector<uint32_t> > m_xSegm;
            std::vector<std::vector<uint32_t> > m_ySegm;
            std::vector<std::vector<uint32_t> > m_zSegm;
            std::vector<uint32_t> m_indices;
            std::vector<uint32_t> m_foundx;
            std::vector<uint32_t> m_foundy;

            uint32_t m_iterId;
            int32_t m_current;
            uint32_t m_facetCount;

            NvcVec3 m_minimal;
            NvcVec3 m_maximal;

            NvcVec3 m_rescale;
    

        };


        /**
            Accelerator which builds map from 3d grid to initial mesh facets. 
            To find all facets which possibly intersect given one, it return all facets which are pointed by grid cells, which intersects with bounding box of given facet.
            To find all facets which possibly cover given point, all facets which are pointed by cells in column which contains given point are returned.
        */
        class BBoxBasedAccelerator : public SpatialAccelerator
        {
        public:
            /**
                \param[in] mesh Mesh for which acceleration structure should be built.
                \param[in] resolution Resolution on 3d grid.
            */
            BBoxBasedAccelerator(const Mesh* mesh, int32_t resolution);
            virtual ~BBoxBasedAccelerator();
            virtual void    release() override;

            virtual int32_t getNextFacet() override;
            virtual void    setState(const Vertex* pos, const Edge* ed, const Facet& fc) override;
            virtual void    setState(const NvcBounds3* bounds) override;
            virtual void    setState(const NvcVec3& p) override;
            virtual void    setPointCmpDirection(int32_t dir) override { NV_UNUSED(dir); }

        private:

            void buildAccelStructure(const Vertex* pos, const Edge* edges, const Facet* fc, int32_t facetCount);

            int32_t m_resolution;
            NvcBounds3 m_bounds;
            std::vector< std::vector<int32_t> > m_spatialMap;
            std::vector<NvcBounds3> m_cells;

    
            // Iterator data
            std::vector<uint32_t> m_alreadyGotFlag;
            uint32_t m_alreadyGotValue;
            std::vector<int32_t> m_cellList;
            int32_t m_gotCells;
            int32_t m_iteratorCell;
            int32_t m_iteratorFacet;
        };

    } // namespace Blast
} // namsepace Nv


#endif // ifndef NVBLASTEXTAUTHORINGACCELERATORIMPL_H
