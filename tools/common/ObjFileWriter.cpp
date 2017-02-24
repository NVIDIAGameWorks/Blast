#include "ObjFileWriter.h"
#include <PxVec3.h>
#include <sstream>

using namespace physx;
using namespace Nv::Blast;

bool ObjFileWriter::saveToFile(const NvBlastAsset* asset, std::vector<std::vector<Nv::Blast::Triangle>> chunksGeometry, std::string assetName, std::string outputPath)
{
	NV_UNUSED(asset);

	std::vector<PxVec3> pos;
	std::vector<PxVec3> norm;
	std::vector<PxVec2> tex;
	for (uint32_t vc = 0; vc < chunksGeometry.size(); ++vc)
	{
		std::vector<Triangle>& chunk = chunksGeometry[vc];
		for (uint32_t i = 0; i < chunk.size(); ++i)
		{
			pos.push_back(chunk[i].a.p);
			pos.push_back(chunk[i].b.p);
			pos.push_back(chunk[i].c.p);

			norm.push_back(chunk[i].a.n);
			norm.push_back(chunk[i].b.n);
			norm.push_back(chunk[i].c.n);

			tex.push_back(chunk[i].a.uv[0]);
			tex.push_back(chunk[i].b.uv[0]);
			tex.push_back(chunk[i].c.uv[0]);
		}
	}
	std::vector < std::vector<std::vector<int32_t> > > indices(chunksGeometry.size());
	int32_t index = 0;
	for (uint32_t vc = 0; vc < chunksGeometry.size(); ++vc)
	{
		indices[vc].push_back(std::vector<int32_t>());
		for (uint32_t i = 0; i < chunksGeometry[vc].size() * 3; ++i)
		{
			indices[vc][0].push_back(index);
			index++;
		}
	}

	return saveToFile(asset, assetName, outputPath, pos, norm, tex, indices);
}

bool ObjFileWriter::saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm,
	const std::vector<physx::PxVec2>& uvs,
	const std::vector<std::vector<std::vector<int32_t> > >& posIndex,
	const std::vector<std::vector<std::vector<int32_t> > >& normIndex,
	const std::vector<std::vector<std::vector<int32_t> > >& texIndex,
	const std::vector<std::string>& texPathes,
	const uint32_t submeshCount)
{
	NV_UNUSED(asset);

	uint32_t chunkCount = static_cast<uint32_t>(posIndex.size());
	if (posIndex.size() != normIndex.size() || normIndex.size() != texIndex.size())
	{
		return false;
	}

	// export materials (mtl file)
	{
		std::ostringstream mtlFilePath;
		mtlFilePath << outputPath << "\\" << name << ".mtl";
		FILE* f = fopen(mtlFilePath.str().c_str(), "w");
		if (!f)
			return false;

		for (uint32_t submeshIndex = 0; submeshIndex < submeshCount; ++submeshIndex)
		{
			fprintf(f, "newmtl mat%d\n", submeshIndex);
			fprintf(f, "\tmap_Kd %s\n", texPathes[submeshIndex].data());
			fprintf(f, "\n");
		}

		fclose(f);
	}

	/// Export geometry to *.obj file
	{
		std::ostringstream objFilePath;
		objFilePath << outputPath << "\\" << name << ".obj";
		FILE* f = fopen(objFilePath.str().c_str(), "w");
		if (!f)
			return false;

		fprintf(f, "mtllib %s.mtl\n", name.c_str());
		fprintf(f, "o frac \n");


		/// Write compressed vertices
		for (uint32_t i = 0; i < pos.size(); ++i)
		{
			fprintf(f, "v %.4f %.4f %.4f\n", pos[i].x, pos[i].y, pos[i].z);
		}
		for (uint32_t i = 0; i < norm.size(); ++i)
		{
			fprintf(f, "vn %.4f %.4f %.4f\n", norm[i].x, norm[i].y, norm[i].z);
		}
		for (uint32_t i = 0; i < uvs.size(); ++i)
		{
			fprintf(f, "vt %.4f %.4f\n", uvs[i].y, uvs[i].x);
		}

		for (uint32_t chunkIndex = 0; chunkIndex < chunkCount; ++chunkIndex)
		{
			for (uint32_t submeshIndex = 0; submeshIndex < posIndex[chunkIndex].size(); ++submeshIndex)
			{
				fprintf(f, "g %d_%d \n", chunkIndex, submeshIndex);
				fprintf(f, "usemtl mat%d\n", submeshIndex);
				uint32_t indexCount = static_cast<uint32_t>(posIndex[chunkIndex][submeshIndex].size());
				const std::vector<int32_t>& pI = posIndex[chunkIndex][submeshIndex];
				const std::vector<int32_t>& nI = normIndex[chunkIndex][submeshIndex];
				const std::vector<int32_t>& tI = texIndex[chunkIndex][submeshIndex];

				for (uint32_t i = 0; i < indexCount;)
				{
					fprintf(f, "f %d/%d/%d  ", pI[i] + 1, tI[i] + 1, nI[i] + 1);
					++i;
					fprintf(f, "%d/%d/%d  ", pI[i] + 1, tI[i] + 1, nI[i] + 1);
					++i;
					fprintf(f, "%d/%d/%d\n", pI[i] + 1, tI[i] + 1, nI[i] + 1);
					++i;
				}
			}
		}
		fclose(f);
	}
	return true;

}

bool ObjFileWriter::saveToFile(const NvBlastAsset* asset, const std::string& name, const std::string& outputPath, const std::vector<physx::PxVec3>& pos, const std::vector<physx::PxVec3>& norm, const std::vector<physx::PxVec2>& uvs,
	const std::vector<std::vector<std::vector<int32_t> > >& indices)
{
	std::vector<std::string> matnames;
	matnames.push_back("");
	return saveToFile(asset, name, outputPath, pos, norm, uvs, indices, indices, indices, matnames, 1);
}