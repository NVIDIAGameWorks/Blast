#pragma once

#include "BlastProjectParameters.h"
#include <QtCore/QVector>
#include <set>
class QDomElement;

typedef nvidia::parameterized::BlastProjectParametersNS::ParametersStruct BPParams;
typedef nvidia::parameterized::BlastProjectParametersNS::GraphicsMaterial_Type BPPGraphicsMaterial;
typedef nvidia::parameterized::BlastProjectParametersNS::MaterialAssignments_Type BPPMaterialAssignments;
typedef nvidia::parameterized::BlastProjectParametersNS::GraphicsMesh_Type BPPGraphicsMesh;
typedef nvidia::parameterized::BlastProjectParametersNS::Light_Type BPPLight;
typedef nvidia::parameterized::BlastProjectParametersNS::Camera_Type BPPCamera;
typedef nvidia::parameterized::BlastProjectParametersNS::CameraBookmark_Type BPPCameraBookmark;
typedef nvidia::parameterized::BlastProjectParametersNS::Scene_Type BPPScene;
typedef nvidia::parameterized::BlastProjectParametersNS::Renderer_Type BPPRenderer;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastFileReferences_Type BPPFileReferences;
typedef nvidia::parameterized::BlastProjectParametersNS::StressSolver_Type BPPStressSolver;
typedef nvidia::parameterized::BlastProjectParametersNS::SupportStructure_Type BPPSupportStructure;
typedef nvidia::parameterized::BlastProjectParametersNS::Bond_Type BPPBond;
typedef nvidia::parameterized::BlastProjectParametersNS::Chunk_Type BPPChunk;
typedef nvidia::parameterized::BlastProjectParametersNS::DamageStruct_Type BPPDamageStruct;
typedef nvidia::parameterized::BlastProjectParametersNS::DefaultDamage_Type BPPDefaultDamage;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAsset_Type BPPAsset;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAssetInstance_Type BPPAssetInstance;
typedef nvidia::parameterized::BlastProjectParametersNS::Blast_Type BPPBlast;
typedef nvidia::parameterized::BlastProjectParametersNS::FractureGeneral_Type BPPFractureGeneral;
typedef nvidia::parameterized::BlastProjectParametersNS::FractureVisualization_Type BPPFractureVisualization;
typedef nvidia::parameterized::BlastProjectParametersNS::Voronoi_Type BPPVoronoi;
typedef nvidia::parameterized::BlastProjectParametersNS::Slice_Type BPPSlice;
typedef nvidia::parameterized::BlastProjectParametersNS::Fracture_Type BPPFracture;
typedef nvidia::parameterized::BlastProjectParametersNS::Filter_Type BPPFilter;

typedef nvidia::parameterized::BlastProjectParametersNS::CameraBookmark_DynamicArray1D_Type BPPBookmarkArray;
typedef nvidia::parameterized::BlastProjectParametersNS::STRING_DynamicArray1D_Type BPPStringArray;
typedef nvidia::parameterized::BlastProjectParametersNS::GraphicsMaterial_DynamicArray1D_Type BPPGraphicsMaterialArray;
typedef nvidia::parameterized::BlastProjectParametersNS::MaterialAssignments_DynamicArray1D_Type BPPMaterialAssignmentsArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Light_DynamicArray1D_Type BPPLightArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Chunk_DynamicArray1D_Type BPPChunkArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Bond_DynamicArray1D_Type BPPBondArray;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAsset_DynamicArray1D_Type BPPAssetArray;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAssetInstance_DynamicArray1D_Type BPPAssetInstanceArray;
typedef nvidia::parameterized::BlastProjectParametersNS::I32_DynamicArray1D_Type BPPI32Array;
typedef nvidia::parameterized::BlastProjectParametersNS::VEC3_DynamicArray1D_Type BPPVEC3Array;
typedef nvidia::parameterized::BlastProjectParametersNS::VEC2_DynamicArray1D_Type BPPVEC2Array;

void freeString(NvParameterized::DummyStringStruct& str);
void freeBlast(BPPGraphicsMesh& data);
void freeBlast(BPPChunk& data);
void freeBlast(BPPBond& data);
void freeBlast(BPPAsset& data);
void freeBlast(BPPAssetInstance& data);
void freeBlast(BPPBlast& data);
void freeBlast(BPPGraphicsMaterial& data);
void freeBlast(BPPDefaultDamage& data);

void freeBlast(BPPStringArray& data);
void freeBlast(BPPChunkArray& data);
void freeBlast(BPPBondArray& data);
void freeBlast(BPPAssetArray& data);
void freeBlast(BPPAssetInstanceArray& data);
void freeBlast(BPPGraphicsMaterialArray& data);

void copy(NvParameterized::DummyStringStruct& dest, const char* source);
void copy(NvParameterized::DummyStringStruct& dest, NvParameterized::DummyStringStruct& source);
bool isItemExist(BPPStringArray& dest, const char* item);
void addItem(BPPStringArray& dest, const char* item);
void removeItem(BPPStringArray& dest, const char* item);
void copy(BPPStringArray& dest, BPPStringArray& source);
void copy(BPPGraphicsMaterialArray& dest, BPPGraphicsMaterialArray& source);
void copy(BPPMaterialAssignmentsArray& dest, BPPMaterialAssignmentsArray& source);
void copy(BPPBookmarkArray& dest, BPPBookmarkArray& source);
void copy(BPPLightArray& dest, BPPLightArray& source);
void copy(BPPChunkArray& dest, BPPChunkArray& source);
void copy(BPPBondArray& dest, BPPBondArray& source);
void copy(BPPAssetArray& dest, BPPAssetArray& source);
void copy(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source);
void copy(BPPI32Array& dest, BPPI32Array& source);
void copy(BPPVEC3Array& dest, BPPVEC3Array& source);
void copy(BPPVEC2Array& dest, BPPVEC2Array& source);

void copy(BPPLight& dest, BPPLight& source);
void copy(BPPRenderer& dest, BPPRenderer& source);
void copy(BPPGraphicsMaterial& dest, BPPGraphicsMaterial& source);
void copy(BPPGraphicsMesh& dest, BPPGraphicsMesh& source);
void copy(BPPMaterialAssignments& dest, BPPMaterialAssignments& source);
void copy(BPPChunk& dest, BPPChunk& source);
void copy(BPPBond& dest, BPPBond& source);
void copy(BPPSupportStructure& dest, BPPSupportStructure& source);
void copy(BPPStressSolver& dest, BPPStressSolver& source);
void copy(BPPAsset& dest, BPPAsset& source);
void copy(BPPAssetInstance& dest, BPPAssetInstance& source);
void copy(BPPBlast& dest, BPPBlast& source);
void copy(BPPFractureGeneral& dest, BPPFractureGeneral& source);
void copy(BPPVoronoi& dest, BPPVoronoi& source);
void copy(BPPFracture& dest, BPPFracture& source);
void copy(BPPDefaultDamage& dest, BPPDefaultDamage& source);
void copy(BPPFilter& dest, BPPFilter& source);
void copy(BPParams& dest, BPParams& source);

void merge(BPPAssetArray& dest, BPPAssetArray& source);
void merge(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source);
void merge(BPPChunkArray& dest, BPPChunkArray& source);
void merge(BPPBondArray& dest, BPPBondArray& source);

/*
void apart(BPPAssetArray& dest, BPPAssetArray& source);
void apart(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source);
void apart(BPPChunkArray& dest, BPPChunkArray& source);
void apart(BPPBondArray& dest, BPPBondArray& source);
*/
void apart(BPPAssetArray& dest, int32_t assetId);
void apart(BPPAssetInstanceArray& dest, int32_t assetId);
void apart(BPPAssetInstanceArray& dest, int32_t assetId, const char* instanceName);
void apart(BPPChunkArray& dest, int32_t assetId);
void apart(BPPBondArray& dest, int32_t assetId);

template <class T>
void init(T& param)
{
	param.buf = nullptr;
	param.arraySizes[0] = 0;
}
void init(BPPStressSolver& param);
void init(BPPGraphicsMaterial& param);
void init(BPPGraphicsMesh& param);
void init(BPPBond& param);
void init(BPPChunk& param);
void init(BPPDefaultDamage& param);
void init(BPPAsset& param);
void init(BPPAssetInstance& param);
void init(BPPVoronoi& param); 
void init(BPPSlice& param);
void init(BPPFractureVisualization& param);
void init(BPParams& params);

enum EFilterRestriction
{
	eFilterRestriction_Invalid = 0x0000,
	eFilterRestriction_AllDescendants = 0x0001 << 0,
	eFilterRestriction_AllParents = 0x0001 << 1,
	eFilterRestriction_DepthAll = 0x0001 << 2,
	eFilterRestriction_Depth0 = 0x0001 << 3,
	eFilterRestriction_Depth1 = 0x0001 << 4,
	eFilterRestriction_Depth2 = 0x0001 << 5,
	eFilterRestriction_Depth3 = 0x0001 << 6,
	eFilterRestriction_Depth4 = 0x0001 << 7,
	eFilterRestriction_Depth5 = 0x0001 << 8,
	eFilterRestriction_ItemTypeAll = 0x0001 << 9,
	eFilterRestriction_Chunk = 0x0001 << 10,
	eFilterRestriction_SupportChunk = 0x0001 << 11,
	eFilterRestriction_StaticSupportChunk = 0x0001 << 12,
	eFilterRestriction_Bond = 0x0001 << 13,
	eFilterRestriction_WorldBond = 0x0001 << 14,
	eFilterRestriction_EqualTo = 0x0001 << 15,
	eFilterRestriction_NotEquaTo = 0x0001 << 16,
	
};

const char* convertFilterRestrictionToString(EFilterRestriction& restriction);
EFilterRestriction convertStringToFilterRestriction(const char* restriction);

struct FilterPreset
{
	FilterPreset(const char* inName);
	std::string name;
	std::vector<EFilterRestriction> filters;
};

struct StressSolverUserPreset
{
	StressSolverUserPreset(const char* inName);
	std::string name;
	BPPStressSolver stressSolver;
};

enum FractureType
{
	eFractureType_Voronoi,
	eFractureType_Slice,
};
struct FracturePreset
{
	FracturePreset(const char* inName, FractureType inType);

	void setType(FractureType inType);

	FractureType type;
	std::string name;
	
	void init();
	union
	{
		BPPVoronoi voronoi;
		BPPSlice slice;
	} fracture;

	BPPFractureVisualization visualization;
};

class BlastProject
{
public:

	static BlastProject& ins();
	~BlastProject();

	BPParams& getParams() { return _projectParams; }

	void clear();

	std::string getAseetNameByID(int assetID);
	int getAssetIDByName(const char* name);
	BPPAsset* getAsset(const char* name);
	int generateNewAssetID();

	bool isGraphicsMaterialNameExist(const char* name);
	BPPGraphicsMaterial* addGraphicsMaterial(const char* name);
	void removeGraphicsMaterial(const char* name);
	void renameGraphicsMaterial(const char* oldName, const char* newName);
	void reloadDiffuseColor(const char* name, float r, float g, float b, float a = 1.0);
	void reloadSpecularColor(const char* name, float r, float g, float b, float a = 1.0);
	void reloadSpecularShininess(const char* name, float specularShininess);
	void reloadDiffuseTexture(const char* name, const char* diffuseTexture);
	void reloadSpecularTexture(const char* name, const char* specularTexture);
	void reloadNormalTexture(const char* name, const char* normalTexture);
	void reloadEnvTexture(const char* name, const char* envTexture);
	BPPGraphicsMaterial* getGraphicsMaterial(const char* name);
	std::string generateNewMaterialName(const char* name);

	bool isAssetInstanceNameExist(const char* name);
	int getAssetInstanceCount(int assetID);
	void getAssetInstances(int assetID, std::vector<BPPAssetInstance*>& instances);
	BPPAssetInstance* getAssetInstance(int assetID, int instanceIndex);
	BPPAssetInstance* getAssetInstance(int assetID, const char* instanceName);
	BPPAssetInstance* addAssetInstance(int blastAssetIndex, const char* instanceName);
	void removeAssetInstance(const char* name);

	BPPChunk* getChunk(BPPAsset& asset, int chunkID);
	std::vector<BPPChunk*> getChildrenChunks(BPPAsset& asset, int parentID);
	std::vector<BPPChunk*> getChildrenChunks(BPPAsset& asset);
	std::vector<BPPChunk*> getChildrenChunks(int assetID);
	std::vector<BPPBond*> getBondsByChunk(BPPAsset& asset, int chunkID);
	std::vector<BPPBond*> getChildrenBonds(BPPAsset& asset);

	bool isUserPresetNameExist(const char* name);
	std::vector<StressSolverUserPreset>& getUserPresets();
	void addUserPreset(const char* name);
	void saveUserPreset();
	void loadUserPreset();

	bool isFracturePresetNameExist(const char* name);
	std::vector<FracturePreset>& getFracturePresets();
	void addFracturePreset(const char* name, FractureType type);
	void saveFracturePreset();
	void loadFracturePreset();

	bool isFilterPresetNameExist(const char* name);
	std::vector<FilterPreset>& getFilterPresets();
	void addFilterPreset(const char* name);
	void removeFilterPreset(const char* name);
	FilterPreset* getFilterPreset(const char* name);
	void renameFilterPreset(const char* oldName, const char* newName);
	void addFilterRestriction(const char* filterName, EFilterRestriction restriction);
	void removeFilterRestriction(const char* filterName, EFilterRestriction restriction);
	void saveFilterPreset();
	void loadFilterPreset();

private:
	BlastProject();
	void _saveStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset);
	void _saveStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver);
	void _loadStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset);
	void _loadStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver);

	void _saveFracturePreset(QDomElement& parentElm, FracturePreset& fracturePreset);
	void _saveFracture(QDomElement& parentElm, BPPVoronoi& voronoi);
	void _saveFracture(QDomElement& parentElm, BPPSlice& slice);
	void _loadFracturePreset(QDomElement& parentElm, FracturePreset& fracturePreset);
	void _loadFracture(QDomElement& parentElm, BPPVoronoi& voronoi);
	void _loadFracture(QDomElement& parentElm, BPPSlice& slice);

	void _saveFilterPreset(QDomElement& parentElm, FilterPreset& filterPreset);
	void _saveRestriction(QDomElement& parentElm, EFilterRestriction& restriction);
	void _loadFilterPreset(QDomElement& parentElm, FilterPreset& filterPreset);
	void _loadRestriction(QDomElement& parentElm, EFilterRestriction& restriction);

	void _addStringItem(BPPStringArray& theArray, const char* name);
	void _removeStringItem(BPPStringArray& theArray, const char* name);

private:
	BPParams	_projectParams;
	std::vector<StressSolverUserPreset> _userPresets;
	std::vector<FracturePreset> _fracturePresets;
	std::vector<FilterPreset> _filterPresets;
};

bool CreateProjectParamsContext();
void ReleaseProjectParamsContext();
bool ProjectParamsLoad(const char* filePath,
                       class SimpleScene* scene);
bool ProjectParamsSave(const char* filePath,
                       class SimpleScene* scene);

bool ParamGetChild(NvParameterized::Handle& parentHandle, NvParameterized::Handle& outChildHandle, const char* childName);
