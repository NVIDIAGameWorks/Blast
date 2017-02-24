#ifndef OBJ_APEX_EXPORTER_H
#define OBJ_APEX_EXPORTER_H

#include <string>
#include <vector>
#include "DestructibleAsset.h"




namespace physx
{
	class PxVec3;
	class PxVec2;
}

struct NvBlastAsset;

namespace Nv
{
namespace Blast
{

class ApexDestructibleGeometryExporter
{
public:
	ApexDestructibleGeometryExporter(std::string materialsDir, std::string exportDir) : m_materialsDir(materialsDir), m_exportDir(exportDir)
	{}

	bool exportToFile(NvBlastAsset* asset, const nvidia::apex::DestructibleAsset& apexAsset, const std::string& name, const std::vector<uint32_t>& chunkReorderInvMap, bool toFbx, bool toObj, bool fbxascii, bool toUe4);

private:
	ApexDestructibleGeometryExporter& operator=(const ApexDestructibleGeometryExporter&)
	{
		return *this;
	}
	const std::string m_materialsDir;
	const std::string m_exportDir;

};


} // namespace Blast
} // namespace Nv


#endif //OBJ_EXPORTER_H
