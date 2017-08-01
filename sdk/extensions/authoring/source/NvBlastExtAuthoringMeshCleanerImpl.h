#include "NvBlastExtAuthoringMeshCleaner.h"

namespace Nv
{
namespace Blast
{

class Mesh;

class MeshCleanerImpl : public MeshCleaner
{
public:
	/**
	Tries to remove self intersections and open edges in interior of mesh.
	\param[in] mesh Mesh to be cleaned.
	\return Cleaned mesh or nullptr if failed.
	*/
	virtual Mesh* cleanMesh(const Nv::Blast::Mesh* mesh) override;
	virtual void release() override;

	~MeshCleanerImpl() {};
};

}
}