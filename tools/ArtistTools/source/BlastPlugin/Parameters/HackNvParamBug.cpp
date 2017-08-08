#include <string.h>
#include "BlastProjectParameters.h"

int GetHackElementSize(const char* data)
{
	if (strstr(data, "graphicsMesh.materialAssignments") != nullptr)
		return sizeof(nvidia::parameterized::BlastProjectParametersNS::MaterialAssignments_Type);
	if (strstr(data, "graphicsMesh.positions") != nullptr)
		return sizeof(nvidia::NvVec3);
	if (strstr(data, "graphicsMesh.normals") != nullptr)
		return sizeof(nvidia::NvVec3);
	if (strstr(data, "graphicsMesh.tangents") != nullptr)
		return sizeof(nvidia::NvVec3);
	if (strstr(data, "graphicsMesh.texcoords") != nullptr)
		return sizeof(nvidia::NvVec2);
	if (strstr(data, "graphicsMesh.positionIndexes") != nullptr)
		return sizeof(int32_t);
	if (strstr(data, "graphicsMesh.normalIndexes") != nullptr)
		return sizeof(int32_t);
	if (strstr(data, "graphicsMesh.texcoordIndexes") != nullptr)
		return sizeof(int32_t);
	if (strstr(data, "graphicsMesh.materialIDs") != nullptr)
		return sizeof(int32_t);
	if (strstr(data, "filter.filters[0].depthFilters") != nullptr)
		return sizeof(uint32_t);
	return 0;
}