#pragma once

#include "BlastProjectParameters.h"
#include <QtCore/QVector>

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
typedef nvidia::parameterized::BlastProjectParametersNS::DefaultDamage_Type BPPDefaultDamage;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAsset_Type BPPAsset;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAssetInstance_Type BPPAssetInstance;
typedef nvidia::parameterized::BlastProjectParametersNS::Projectile_Type BPPProjectile;
typedef nvidia::parameterized::BlastProjectParametersNS::Landmark_Type BPPLandmark;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastComposite_Type BPPComposite;
typedef nvidia::parameterized::BlastProjectParametersNS::Blast_Type BPPBlast;
typedef nvidia::parameterized::BlastProjectParametersNS::FractureGeneral_Type BPPFractureGeneral;
typedef nvidia::parameterized::BlastProjectParametersNS::FractureVisualization_Type BPPFractureVisualization;
typedef nvidia::parameterized::BlastProjectParametersNS::ShellCut_Type BPPShellCut;
typedef nvidia::parameterized::BlastProjectParametersNS::Voronoi_Type BPPVoronoi;
typedef nvidia::parameterized::BlastProjectParametersNS::Slice_Type BPPSlice;
typedef nvidia::parameterized::BlastProjectParametersNS::CutoutProjection_Type BPPCutoutProjection;
typedef nvidia::parameterized::BlastProjectParametersNS::Fracture_Type BPPFracture;
typedef nvidia::parameterized::BlastProjectParametersNS::FilterPreset_Type BPPFilterPreset;
typedef nvidia::parameterized::BlastProjectParametersNS::Filter_Type BPPFilter;

typedef nvidia::parameterized::BlastProjectParametersNS::CameraBookmark_DynamicArray1D_Type BPPBookmarkArray;
typedef nvidia::parameterized::BlastProjectParametersNS::STRING_DynamicArray1D_Type BPPStringArray;
typedef nvidia::parameterized::BlastProjectParametersNS::GraphicsMaterial_DynamicArray1D_Type BPPGraphicsMaterialArray;
typedef nvidia::parameterized::BlastProjectParametersNS::GraphicsMesh_DynamicArray1D_Type BPPGraphicsMeshArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Light_DynamicArray1D_Type BPPLightArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Chunk_DynamicArray1D_Type BPPChunkArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Bond_DynamicArray1D_Type BPPBondArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Projectile_DynamicArray1D_Type BPPProjectileArray;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAsset_DynamicArray1D_Type BPPAssetArray;
typedef nvidia::parameterized::BlastProjectParametersNS::BlastAssetInstance_DynamicArray1D_Type BPPAssetInstanceArray;
typedef nvidia::parameterized::BlastProjectParametersNS::Landmark_DynamicArray1D_Type BPPLandmarkArray;
typedef nvidia::parameterized::BlastProjectParametersNS::FilterPreset_DynamicArray1D_Type BPPFilterPresetArray;
typedef nvidia::parameterized::BlastProjectParametersNS::U32_DynamicArray1D_Type BPPU32Array;

void freeString(NvParameterized::DummyStringStruct& str);
void freeBlast(BPPGraphicsMesh& data);
void freeBlast(BPPChunk& data);
void freeBlast(BPPBond& data);
void freeBlast(BPPAsset& data);
void freeBlast(BPPAssetInstance& data);
void freeBlast(BPPComposite& data);
void freeBlast(BPPBlast& data);
void freeBlast(BPPLandmark& data);

void freeBlast(BPPStringArray& data);
void freeBlast(BPPGraphicsMeshArray& data);
void freeBlast(BPPChunkArray& data);
void freeBlast(BPPBondArray& data);
void freeBlast(BPPAssetArray& data);
void freeBlast(BPPAssetInstanceArray& data);
void freeBlast(BPPLandmarkArray& data);

void copy(NvParameterized::DummyStringStruct& dest, const char* source);
void copy(NvParameterized::DummyStringStruct& dest, NvParameterized::DummyStringStruct& source);
void copy(BPPStringArray& dest, BPPStringArray& source);
void copy(BPPGraphicsMaterialArray& dest, BPPGraphicsMaterialArray& source);
void copy(BPPGraphicsMeshArray& dest, BPPGraphicsMeshArray& source);
void copy(BPPBookmarkArray& dest, BPPBookmarkArray& source);
void copy(BPPLightArray& dest, BPPLightArray& source);
void copy(BPPChunkArray& dest, BPPChunkArray& source);
void copy(BPPBondArray& dest, BPPBondArray& source);
void copy(BPPProjectileArray& dest, BPPProjectileArray& source);
void copy(BPPAssetArray& dest, BPPAssetArray& source);
void copy(BPPAssetInstanceArray& dest, BPPAssetInstanceArray& source);
void copy(BPPLandmarkArray& dest, BPPLandmarkArray& source);
void copy(BPPU32Array& dest, BPPU32Array& source);
void copy(BPPFilterPresetArray& dest, BPPFilterPresetArray& source);

void copy(BPPLight& dest, BPPLight& source);
void copy(BPPRenderer& dest, BPPRenderer& source);
void copy(BPPGraphicsMaterial& dest, BPPGraphicsMaterial& source);
void copy(BPPGraphicsMesh& dest, BPPGraphicsMesh& source);
void copy(BPPMaterialAssignments& dest, BPPMaterialAssignments& source);
void copy(BPPChunk& dest, BPPChunk& source);
void copy(BPPBond& dest, BPPBond& source);
void copy(BPPProjectile& dest, BPPProjectile& source);
void copy(BPPSupportStructure& dest, BPPSupportStructure& source);
void copy(BPPLandmark& dest, BPPLandmark& source);
void copy(BPPStressSolver& dest, BPPStressSolver& source);
void copy(BPPAsset& dest, BPPAsset& source);
void copy(BPPAssetInstance& dest, BPPAssetInstance& source);
void copy(BPPComposite& dest, BPPComposite& source);
void copy(BPPBlast& dest, BPPBlast& source);
void copy(BPPFilter& dest, BPPFilter& source);
void copy(BPPVoronoi& dest, BPPVoronoi& source);
void copy(BPPCutoutProjection& dest, BPPCutoutProjection& source);
void copy(BPPFracture& dest, BPPFracture& source);
void copy(BPPFilterPreset& dest, BPPFilterPreset& source);
void copy(BPParams& dest, BPParams& source);

void merge(BPPChunkArray& dest, BPPChunkArray& source);
void merge(BPPBondArray& dest, BPPBondArray& source);

void init(BPPStressSolver& param);
void init(BPPGraphicsMaterial& param);
void init(BPParams& params);

struct StressSolverUserPreset
{
	StressSolverUserPreset(const char* inName);
	std::string name;
	BPPStressSolver stressSolver;
};

class BlastProject
{
public:

	static BlastProject& ins();
	~BlastProject();

	BPParams& getParams() { return _projectParams; }

	void clear();

	bool isGraphicsMaterialNameExist(const char* name);
	BPPGraphicsMaterial* addGraphicsMaterial(const char* name, const char* diffuseTexture);
	void removeGraphicsMaterial(const char* name);
	void renameGraphicsMaterial(const char* oldName, const char* newName);

	std::vector<BPPAsset*> getSelectedBlastAssets(void);

	bool isAssetInstanceNameExist(const char* name);
	BPPAssetInstance* getAssetInstance(const char* assetPath, int instanceIndex);
	BPPAssetInstance* addAssetInstance(int blastAssetIndex, const char* instanceName);
	void removeAssetInstance(const char* name);

	BPPChunk* getChunk(BPPAsset& asset, int id);
	std::vector<BPPChunk*> getChildrenChunks(BPPAsset& asset, int parentID);
	std::vector<BPPChunk*> getChildrenChunks(BPPAsset& asset);
	std::vector<BPPBond*> getBondsByChunk(BPPAsset& asset, int chunkID);

	bool isLandmarkNameExist(const char* name);
	BPPLandmark* addLandmark(const char* name);
	void removeLandmark(const char* name);
	BPPLandmark* getLandmark(const char* name);
	void renameLandmark(const char* oldName, const char* newName);

	bool isUserPresetNameExist(const char* name);
	std::vector<StressSolverUserPreset>& getUserPresets();
	void addUserPreset(const char* name);
	void saveUserPreset();
	void loadUserPreset();

	bool isFilterPresetNameExist(const char* name);
	std::vector<BPPFilterPreset*> getFilterPresets();
	void addFilterPreset(const char* name);
	void removeFilterPreset(const char* name);
	BPPFilterPreset* getFilterPreset(const char* name);
	void renameFilterPreset(const char* oldName, const char* newName);
	void addFilterDepth(const char* filterName, int depth);
	void removeFilterDepth(const char* filterName, int depth);

	bool isCutoutTextureNameExist(const char* name);
	void addCutoutTexture(const char* name);
	void removeCutoutTexture(const char* name);

	bool isPaintMaskNameExist(const char* name);
	void addPaintMasks(const char* name);
	void removePaintMasks(const char* name);

	bool isMeshCutterNameExist(const char* name);
	void addMeshCutter(const char* name);
	void removeMeshCutter(const char* name);

	bool isVoronoiTextureNameExist(const char* name);
	void addVoronoiTexture(const char* name);
	void removeVoronoiTexture(const char* name);

private:
	BlastProject();
	void _saveStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset);
	void _saveStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver);
	void _loadStressSolverPreset(QDomElement& parentElm, StressSolverUserPreset& stressSolverUserPreset);
	void _loadStressSolver(QDomElement& parentElm, BPPStressSolver& stressSolver);

	void _addStringItem(BPPStringArray& theArray, const char* name);
	void _removeStringItem(BPPStringArray& theArray, const char* name);

private:
	BPParams	_projectParams;
	std::vector<StressSolverUserPreset> _userPresets;
};

bool CreateProjectParamsContext();
void ReleaseProjectParamsContext();
bool ProjectParamsLoad(const char* filePath,
                       class SimpleScene* scene);
bool ProjectParamsSave(const char* filePath,
                       class SimpleScene* scene);

bool ParamGetChild(NvParameterized::Handle& parentHandle, NvParameterized::Handle& outChildHandle, const char* childName);
