{
	header =>
	{
		className => 'BlastProjectParameters',
		implementStorage => 1,
	
		# Version history
		# 0.0 Initial Version
		classVersion => '0.0',

		hints =>
		{
		},
	},

	structs =>
	[
		{
			name => 'GraphicsMaterial',
			parameters =>
			[
				{
					name => 'ID',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "ID of this material" },
				},
				{
					name => 'name',
					type => 'STRING',
					hints => { shortDescription => 'Name of this material' },
				},
				{
					name => 'useTextures',
					type => 'BOOL',
					hints => { shortDescription => 'Use textures'},
				},
				{
					name => 'diffuseTextureFilePath',
					type => 'STRING',
					hints => { shortDescription => 'Diffuse texture file path' },
				},
				{
					name => 'specularTextureFilePath',
					type => 'STRING',
					hints => { shortDescription => 'Specular texture file path' },
				},
				{
					name => 'normalTextureFilePath',
					type => 'STRING',
					hints => { shortDescription => 'Normal texture file path' },
				},
				{
					name => 'diffuseColor',
					type => 'VEC4',
					defaultValue => '0',
					hints => { shortDescription => "Diffuse color" },
				},
				{
					name => 'specularColor',
					type => 'VEC4',
					defaultValue => '0',
					hints => { shortDescription => "Specular color" },
				},
				{
					name => 'specularShininess',
					type => 'F32',
					defaultValue => '1.0f',
					hints => { shortDescription => 'Specular shininess' },
				},
			]
		},
		{
			name => 'MaterialAssignments',
			parameters =>
			[
				{
					name => 'libraryMaterialID',
					type => 'I32',
					defaultValue => '-1',
					hints =>
					{
						shortDescription => "ID of the material in material library",
					},
				},
				{
					name => 'faceMaterialID',
					type => 'I32',
					defaultValue => '-1',
					hints =>
					{
						shortDescription => "ID of the material for face, which are generated in graphics mesh range",
					},
				},
			]
		},
		{
			name => 'GraphicsMesh',
			parameters =>
			[
				{
					name => 'materialAssignments',
					type => 'MaterialAssignments',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "MaterialAssignments",
					},
				},
				{
					name => 'positions',
					type => 'VEC3',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Position data array",
					},
				},
				{
					name => 'normals',
					type => 'VEC3',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Normal data array",
					},
				},
				{
					name => 'tangents',
					type => 'VEC3',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Tangent data array",
					},
				},
				{
					name => 'texcoords',
					type => 'VEC2',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Texcoord data array",
					},
				},
				{
					name => 'vertextCountInFace',
					type => 'U32',
					defaultValue => '3',
					hints =>
					{
						shortDescription => "Count of vertextes of one face",
					},
				},
				{
					name => 'positionIndexes',
					type => 'I32',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Indexes of the positions of each face",
					},
				},
				{
					name => 'normalIndexes',
					type => 'I32',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Indexes of the normal of each face",
					},
				},
				{
					name => 'tangentIndexes',
					type => 'I32',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Indexes of the tangents of each face",
					},
				},
				{
					name => 'texcoordIndexes',
					type => 'I32',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Indexes of the texcoords of each face",
					},
				},
				{
					name => 'materialIDs',
					type => 'I32',
					isArray => 1,
					arraySize => '-1',
					hints => { shortDescription => "IDs of the material specified for each face " },
				},
			]
		},
		{
			name => 'Light',
			parameters =>
			[
				{
					name => 'name',
					type => 'STRING',
					defaultValue => '',
					hints => { shortDescription => 'name of light' },
				},
				{
					name => 'enable',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "enable this light" },
				},
				{
					name => 'useShadows',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "use shadows for this light" },
				},
				{
					name => 'lockToRoot',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "lock this light to the root bone" },
				},
				{
					name => 'visualize',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "visualize this light" },
				},
				{
					name => 'type',
					type => 'I32',
					defaultValue => '0',
					hints =>
					{
						shortDescription => "Type of this light",
					},
				},
				{
					name => 'shadowMapResolution',
					type => 'I32',
					defaultValue => '0',
					hints =>
					{
						shortDescription => "shadow resolution",
					},
				},
				{
					name => 'color',
					type => 'VEC3',
					defaultValue => '1.0f',
					hints => { shortDescription => "Light color for visualization" },
				},
				{
					name => 'diffuseColor',
					type => 'VEC3',
					defaultValue => '1.0f',
					hints => { shortDescription => "Light diffuse color" },
				},
				{
					name => 'ambientColor',
					type => 'VEC3',
					defaultValue => '1.0f',
					hints => { shortDescription => "Light ambient color" },
				},
				{
					name => 'specularColor',
					type => 'VEC3',
					defaultValue => '1.0f',
					hints => { shortDescription => "Light specular color" },
				},
				{
					name => 'intensity',
					type => 'F32',
					defaultValue => '1.0f',
					hints => { shortDescription => "Light intensity" },
				},
				{
					name => 'distance',
					type => 'F32',
					defaultValue => '100.0f',
					hints => { shortDescription => "Light distance (for position based lights)" },
				},
				{
					name => 'spotFalloffStart',
					type => 'F32',
					defaultValue => '20.0f',
					hints => { shortDescription => "Fall off start angle for spot light" },
				},
				{
					name => 'spotFalloffEnd',
					type => 'F32',
					defaultValue => '30.0f',
					hints => { shortDescription => "Fall off end angle for spot light" },
				},
				{
					name => 'lightAxisX',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "X axis of light matrix",
					},
				},
				{
					name => 'lightAxisY',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Y axis of light matrix",
					},
				},
				{
					name => 'lightAxisZ',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Z axis of light matrix",
					},
				},
				{
					name => 'lightPos',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "light position",
					},
				},
			]
		},
		{
			name => 'Camera',
			parameters =>
			[
				{
					name => 'flags',
					type => 'U16',
					defaultValue => '0',
					hints => { shortDescription => "Y Up(1) or Z Up(2)" },
				},
				{
					name => 'fov',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "FOV" },
				},
				{
					name => 'aspectRatio',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "FOV" },
				},
				{
					name => 'znear',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Near Z" },
				},
				{
					name => 'zfar',
					type => 'F32',
					defaultValue => '1.0f',
					hints => { shortDescription => "Far Z" },
				},
				{
					name => 'width',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Width for Ortho" },
				},
				{
					name => 'height',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Height for Ortho" },
				},
				{
					name => 'isPerspective',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Camera Eye Position" },
				},
				{
					name => 'eye',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints => { shortDescription => "Camera Eye Position" },
				},
				{
					name => 'at',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints => { shortDescription => "Camera At Position" },
				},
				{
					name => 'xAxis',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints => { shortDescription => "X Axis" },
				},
				{
					name => 'yAxis',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints => { shortDescription => "Y Axis" },
				},
				{
					name => 'zAxis',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints => { shortDescription => "Z Axis" },
				},
				{
					name => 'viewDirection',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints => { shortDescription => "View Direction" },
				},
				{
					name => 'lookDistance',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Look Distance" },
				},
				{
					name => 'orientation',
					type => 'VEC4',
					defaultValue => '0.0f',
					hints => { shortDescription => "Orientation Quaternion" },
				},
				{
					name => 'viewMatrix',
					type => 'MAT44',
					defaultValue => '0.0f',
					hints => { shortDescription => "View Matrix" },
				},
				{
					name => 'projectionMatrix',
					type => 'MAT44',
					defaultValue => '0.0f',
					hints => { shortDescription => "View Matrix" },
				},
			]
		},
		{
			name => 'CameraBookmark',
			parameters =>
			[
				{
					name => 'name',
					type => 'STRING',
					hints => { shortDescription => "Name of the bookmark" },
				},
				{
					name => 'camera',
					type => 'Camera',
					hints => { shortDescription => "Camera information" },
				},
			]
		},
		{
			name => 'Scene',
			parameters =>
			[
				{
					name => 'repeatAnimation',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Repeat animation" },
				},
				{
					name => 'animationSpeed',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Animation speed" },
				},
				{
					name => 'showGrid',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Show grid" },
				},
				{
					name => 'showAxis',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Show axis" },
				},
				{
					name => 'upAxis',
					type => 'U32',
					defaultValue => '0',
					hints => { shortDescription => "Up axis" },
				},
				{
					name => 'sceneUnitIndex',
					type => 'U32',
					defaultValue => '0',
					hints => { shortDescription => "Scene Unit" },
				},
			]
		},
		{
			name => 'Renderer',
			parameters =>
			[
				{
					name => 'renderFps',
					type => 'F32',
					defaultValue => '60.0f',
					hints =>
					{
						shortDescription => "Render Play Rate FPS",
					},
				},
				{
					name => 'frameStartTime',
					type => 'F32',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Frame start time",
					},
				},
				{
					name => 'frameEndTime',
					type => 'F32',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Frame end time",
					},
				},
				{
					name => 'animationFps',
					type => 'F32',
					defaultValue => '24.0f',
					hints =>
					{
						shortDescription => "Animation FPS",
					},
				},
				{
					name => 'animate',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Is animated",
					},
				},
				{
					name => 'simulate',
					type => 'BOOL',
					defaultValue => 'true',
					hints =>
					{
						shortDescription => "Is simulated",
					},
				},
				{
					name => 'resetSimulationOnLoop',
					type => 'BOOL',
					defaultValue => 'true',
					hints =>
					{
						shortDescription => "Reset simulation state on loop",
					},
				},
				{
					name => 'simulationFps',
					type => 'F32',
					defaultValue => '60.0f',
					hints =>
					{
						shortDescription => "Simulation Rate FPS",
					},
				},
				{
					name => 'showGraphicsMesh',
					type => 'BOOL',
					defaultValue => 'true',
					hints =>
					{
						shortDescription => "Show graphics mesh",
					},
				},
				{
					name => 'visualizeGrowthMesh',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Visualize growth mesh",
					},
				},
				{
					name => 'visualizeLight',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Visualize light",
					},
				},
				{
					name => 'visualizeWind',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Visualize wind",
					},
				},
				{
					name => 'showStatistics',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Show statistics",
					},
				},
				{
					name => 'renderStyle',
					type => 'I32',
					defaultValue => '2',
					hints =>
					{
						shortDescription => "Render style",
					},
				},
				{
					name => 'colorizeOption',
					type => 'I32',
					defaultValue => '0',
					hints =>
					{
						shortDescription => "Colorize option",
					},
				},
				{
					name => 'showWireframe',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Show wireframe",
					},
				},
				{
					name => 'lockRootBone',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Lock root bone",
					},
				},
				{
					name => 'controlTextureOption',
					type => 'I32',
					defaultValue => '0',
					hints =>
					{
						shortDescription => "Control texture option",
					},
				},
				{
					name => 'useLighting',
					type => 'BOOL',
					defaultValue => 'true',
					hints =>
					{
						shortDescription => "Use lighting",
					},
				},
				{
					name => 'showSkinnedMeshOnly',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Show skinned mesh only",
					},
				},
				{
					name => 'lightDir',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Light direction",
					},
				},
				{
					name => 'ambientColor',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Scene ambient color",
					},
				},
				{
					name => 'windDir',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Wind direction",
					},
				},
				{
					name => 'windStrength',
					type => 'F32',
					defaultValue => '1.0f',
					hints =>
					{
						shortDescription => "Wind strength",
					},
				},
				{
					name => 'lightIntensity',
					type => 'F32',
					defaultValue => '1.0f',
					hints =>
					{
						shortDescription => "Light intensity",
					},
				},
				{
					name => 'gravityDir',
					type => 'VEC3',
					defaultValue => '0.0f',
					hints =>
					{
						shortDescription => "Gravity direction",
					},
				},
				{
					name => 'gravityScale',
					type => 'F32',
					defaultValue => '1.0f',
					hints =>
					{
						shortDescription => "Gravity scale",
					},
				},
				{
					name => 'textureFilePath',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Texture file path",
					},
				},
				{
					name => 'lights',
					type => 'Light',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Light data",
					},
				},
			]
		},
		{
			name => 'BlastFileReferences',
			parameters =>
			[
				{
					name => 'fbxSourceAsset',
					type => 'STRING',
					hints => { shortDescription => "FBX source asset path" },
				},
			]
		},
		{
			name => 'StressSolver',
			parameters =>
			[
				{
					name => 'hardness',
					type => 'F32',
					defaultValue => '1000.0f',
					hints => { shortDescription => "Hardness of bond's material" },
				},
				{
					name => 'linearFactor',
					type => 'F32',
					defaultValue => '0.25f',
					hints => { shortDescription => "Linear stress on bond multiplier" },
				},
				{
					name => 'angularFactor',
					type => 'F32',
					defaultValue => '0.75f',
					hints => { shortDescription => "Angular stress on bond multiplier" },
				},
				{
					name => 'bondIterationsPerFrame',
					type => 'U32',
					defaultValue => '18000',
					hints => { shortDescription => "Number of bond iterations to perform per frame" },
				},
				{
					name => 'graphReductionLevel',
					type => 'U32',
					defaultValue => '3',
					hints => { shortDescription => "Graph reduction level" },
				},
			]
		},
		{
			name => 'SupportStructure',
			parameters =>
			[
				{
					name => 'healthMask',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Name of active health mask",
					},
				},
				{
					name => 'bondStrength',
					type => 'F32',
					defaultValue => '1.0',
					hints => { shortDescription => "Bond strength" },
				},
				{
					name => 'enableJoint',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Whether have a joint or not" },
				},
			]
		},
		{
			name => 'Bond',
			parameters =>
			[
				{
					name => 'name',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Name of bond",
					},
				},
				{
					name => 'asset',
					type => 'I32',
					hints =>
					{
						shortDescription => "ID of the blast asset this bond belongs to",
					},
				},
				{
					name => 'visible',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Is this bond visible" },
				},
				{
					name => 'fromChunk',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "ID of the chunk this bond is from" },
				},
				{
					name => 'toChunk',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "ID of the chunk this bond is to" },
				},
				{
					name => 'support',
					type => 'SupportStructure',
				},
			]
		},
		{
			name => 'Chunk',
			parameters =>
			[
				{
					name => 'ID',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "ID of this chunk" },
				},
				{
					name => 'parentID',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "ID of parent chunk" },
				},
				{
					name => 'name',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Name of chunk",
					},
				},
				{
					name => 'asset',
					type => 'I32',
					hints =>
					{
						shortDescription => "ID of the blast asset this chunk belongs to",
					},
				},
				{
					name => 'visible',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Is this chunk visible" },
				},
				{
					name => 'support',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Is this chunk a support chunk" },
				},
				{
					name => 'staticFlag',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this chunk static" },
				},
				{
					name => 'graphicsMesh',
					type => 'GraphicsMesh',
					hints => { shortDescription => "Graphics mesh of this chunk" },
				},
			]
		},
		{
			name => 'DamageStruct',
			parameters =>
			[
				{
					name => 'damageRadius',
					type => 'F32',
					defaultValue => '5.0f',
					hints => { shortDescription => "Damage radius (Mouse WH)" },
				},
				{
					name => 'continuously',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Damage continuously" },
				},
			]
		},
		{
			name => 'DefaultDamage',
			parameters =>
			[
				{
					name => 'damageAmount',
					type => 'F32',
					defaultValue => '100.0f',
					hints => { shortDescription => "Damage Amount" },
				},
				{
					name => 'explosiveImpulse',
					type => 'F32',
					defaultValue => '100.0f',
					hints => { shortDescription => "Explosive impulse" },
				},
				{
					name => 'stressDamageForce',
					type => 'F32',
					defaultValue => '1.0f',
					hints => { shortDescription => "Stress damage force" },
				},
				{
					name => 'damageProfile',
					type => 'U32',
					defaultValue => '0',
					hints => { shortDescription => "FallOff" },
				},
				{
					name => 'damageStructs',
					type => 'DamageStruct',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Damage Structs",
					},
				},
			]
		},
		{
			name => 'BlastAsset',
			parameters =>
			[
				{
					name => 'ID',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "ID of this asset" },
				},
				{
					name => 'name',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Name of this blast asset",
					},
				},
				{
					name => 'visible',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Is this blast asset visible" },
				},
				{
					name => 'stressSolver',
					type => 'StressSolver',
				},
				{
					name => 'activeUserPreset',
					type => 'STRING',
					hints => { shortDescription => "Name of active user preset" },
				},
				{
					name => 'fbx',
					type => 'STRING',
					hints => { shortDescription => "FBX export asset path" },
				},
				{
					name => 'exportFBX',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this blast asset export FBX" },
				},
				{
					name => 'obj',
					type => 'STRING',
					hints => { shortDescription => "OBJ export asset path" },
				},
				{
					name => 'exportOBJ',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this blast asset export OBJ" },
				},
				{
					name => 'collision',
					type => 'STRING',
					hints => { shortDescription => "Collision export asset path" },
				},
				{
					name => 'exportCollision',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this blast asset export Collision" },
				},
				{
					name => 'llasset',
					type => 'STRING',
					hints => { shortDescription => "LLAsset export asset path" },
				},
				{
					name => 'exportLLAsset',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this blast asset export LLAsset" },
				},
				{
					name => 'tkasset',
					type => 'STRING',
					hints => { shortDescription => "TKAsset export asset path" },
				},
				{
					name => 'exportTKAsset',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this blast asset export TKAsset" },
				},
				{
					name => 'bpxa',
					type => 'STRING',
					hints => { shortDescription => "Blast export asset path" },
				},
				{
					name => 'exportBPXA',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Is this blast asset export BPXA" },
				},
			]
		},
		{
			name => 'Transform',
			parameters =>
			[
				{
					name => 'position',
					type => 'VEC3',
					defaultValue => '0',
					hints => 
					{ 
						shortDescription => "Position" 
					},
				},
				{
					name => 'rotation',
					type => 'VEC4',
					defaultValue => '0',
					hints => 
					{ 
						shortDescription => "Rotation" 
					},
				},
			]
		},
		{
			name => 'BlastAssetInstance',
			parameters =>
			[
				{
					name => 'name',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Name of blast asset instance",
					},
				},
				{
					name => 'visible',
					type => 'BOOL',
					defaultValue => 'true',
					hints => { shortDescription => "Is this bond visible" },
				},
				{
					name => 'asset',
					type => 'I32',
					hints =>
					{
						shortDescription => "ID of the blast asset this instance created by",
					},
				},
				{
					name => 'transform',
					type => 'Transform',
					hints =>
					{
						shortDescription => "Transform of blast asset instance",
					},
				},
				{
					name => 'exMaterial',
					type => 'STRING',
					defaultValue => '',
					hints => { shortDescription => "External material of blast asset instance" },
				},
				{
					name => 'inMaterial',
					type => 'STRING',
					defaultValue => '',
					hints => { shortDescription => "Internal material of blast asset instance" },
				},
			]
		},
		{
			name => 'Blast',
			parameters =>
			[
				{
					name => 'fileReferences',
					type => 'BlastFileReferences',
				},
				{
					name => 'blastAssets',
					type => 'BlastAsset',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Blast assets",
					},
				},
				{
					name => 'blastAssetInstances',
					type => 'BlastAssetInstance',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Blast asset instances",
					},
				},
				{
					name => 'chunks',
					type => 'Chunk',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Chunks",
					},
				},
				{
					name => 'bonds',
					type => 'Bond',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Bonds",
					},
				},
				{
					name => 'healthMask',
					type => 'STRING',
					hints =>
					{
						shortDescription => "Health mask file path",
					},
				},
			]
		},
		{
			name => 'FractureGeneral',
			parameters =>
			[
				{
					name => 'fracturePreset',
					type => 'STRING',
					hints => { shortDescription => "Name of fracture preset" },
				},
				{
					name => 'fractureType',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "Index of fracture type" },
				},
				{
					name => 'applyMaterial',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "Apply material" },
				},
				{
					name => 'autoSelectNewChunks',
					type => 'BOOL',
					defaultValue => 'false',
					hints => { shortDescription => "Auto Select New Chunks" },
				},
				{
					name => 'selectionDepthTest',
					type => 'BOOL',
					defaultValue => 'true',
					hints =>
					{
						shortDescription => "Selection Depth Test",
					},
				},
			]
		},
		{
			name => 'FractureVisualization',
			parameters =>
			[
				{
					name => 'fracturePreview',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Show fracture preview",
					},
				},
				{
					name => 'displayFractureWidget',
					type => 'BOOL',
					defaultValue => 'false',
					hints =>
					{
						shortDescription => "Display fracture widget",
					},
				},
			]
		},
		{
			name => 'Voronoi',
			parameters =>
			[
				{
					name => 'siteGeneration',
					type => 'I32',
					defaultValue => '-1',
					hints => { shortDescription => "Index of site generation" },
				},
				{
					name => 'numSites',
					type => 'U32',
					defaultValue => '5',
					hints => { shortDescription => "Number of generated sites for uniform site generation method" },
				},
				{
					name => 'numberOfClusters',
					type => 'U32',
					defaultValue => '1',
					hints => { shortDescription => "Number of generated clusters" },
				},
				{
					name => 'sitesPerCluster',
					type => 'U32',
					defaultValue => '1.0f',
					hints => { shortDescription => "Number of sites in each cluster" },
				},
				{
					name => 'clusterRadius',
					type => 'F32',
					defaultValue => '1.0f',
					hints => { shortDescription => "Voronoi cells cluster radius" },
				},
			]
		},
		{
			name => 'Slice',
			parameters =>
			[
				{
					name => 'numSlicesX',
					type => 'U32',
					defaultValue => '1',
					hints => { shortDescription => "Number of slices along X axis" },
				},
				{
					name => 'numSlicesY',
					type => 'U32',
					defaultValue => '1',
					hints => { shortDescription => "Number of slices along Z axis" },
				},
				{
					name => 'numSlicesZ',
					type => 'U32',
					defaultValue => '1',
					hints => { shortDescription => "Number of slices along Z axis" },
				},
				{
					name => 'offsetVariation',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Offset of variation" },
				},
				{
					name => 'rotationVariation',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Rotation of variation" },
				},
				{
					name => 'noiseAmplitude',
					type => 'F32',
					defaultValue => '0.0f',
					hints => { shortDescription => "Noise of amplitude" },
				},
				{
					name => 'noiseFrequency',
					type => 'F32',
					defaultValue => '1.0f',
					hints => { shortDescription => "Noise of frequency" },
				},
				{
					name => 'noiseOctaveNumber',
					type => 'U32',
					defaultValue => '1',
					hints => { shortDescription => "Noise octave number, which declares how many octaves of noise will be summed to form final noise function" },
				},
				{
					name => 'noiseSeed',
					type => 'U32',
					defaultValue => '1',
					hints => { shortDescription => "Noise of seed" },
				},
				{
					name => 'surfaceResolution',
					type => 'I32',
					defaultValue => '1',
					hints => { shortDescription => "Cutting surface resolution" },
				},
			]
		},
		{
			name => 'Fracture',
			parameters =>
			[
				{
					name => 'general',
					type => 'FractureGeneral',
				},
				{
					name => 'visualization',
					type => 'FractureVisualization',
				},
				{
					name => 'voronoi',
					type => 'Voronoi',
				},
				{
					name => 'slice',
					type => 'Slice',
				},
			]
		},
		{
			name => 'Filter',
			parameters =>
			[
				{
					name => 'activeFilter',
					type => 'STRING',
					hints => { shortDescription => "Name of active filter preset" },
				},
				{
					name => 'filterRestrictions',
					type => 'STRING',
					isArray => 1,
					arraySize => '-1',
					hints =>
					{
						shortDescription => "Filter restrictions",
					},
				},
			]
		},
	],

	parameters =>
	[
		{
			name => 'camera',
			type => 'Camera',
		},
		{
			name => 'cameraBookmarks',
			type => 'CameraBookmark',
			isArray => 1,
			arraySize => '-1',
			hints => { shortDescription => "All camera bookmarks" },
		},
		{
			name => 'lightCamera',
			type => 'Camera',
		},
		{
			name => 'windCamera',
			type => 'Camera',
		},
		{
			name => 'graphicsMaterials',
			type => 'GraphicsMaterial',
			isArray => 1,
			arraySize => '-1',
			hints =>
			{
				shortDescription => "Graphics materials",
			},
		},
		{
			name => 'scene',
			type => 'Scene',
		},
		{
			name => 'renderer',
			type => 'Renderer',
		},
		{
			name => 'blast',
			type => 'Blast',
		},
		{
			name => 'fracture',
			type => 'Fracture',
		},
		{
			name => 'defaultDamage',
			type => 'DefaultDamage',
		},
		{
			name => 'filter',
			type => 'Filter',
		},
	]
}
