{
	header =>
	{
		className => 'PlaylistParams',
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
	],

	parameters =>
	[
		{
			name => 'blastProjFilePaths',
			type => 'STRING',
			isArray => 1,
			arraySize => '-1',
			hints =>
			{
				shortDescription => "BlastProj file paths",
			},
		},
	]
}
