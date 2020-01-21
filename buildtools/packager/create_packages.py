import packager
import argparse
import os
import sys
import formic
import tempfile
import shutil
import xml.etree.ElementTree as ET

__author__ = 'hfannar'

PLATFORMS = ['windows', 'linux', 'xboxone', 'ps4']

PLATFORM_DIRS = {
    # platform name : platform specific directories (will be excluded on other platforms)
    'ps4': ['**/*ps4*/**', '**/*PS4*/**'],
    'xboxone': ['**/*xboxone*/**', '**/*XboxOne*/**'],
    'windows': ['**/windows/*.cmake', '**/windows/CMakeLists.txt', '**/vc*win*/**'],
    'linux': ['**/unix/**', '**/linux*-gcc/**'],  # I don't know yet what cmake will generate here
}

# Filters work recursively
# Exclude filter has higher priority than include filter
# 'include' filter can disable 'exclude' filter which applied on ancestor folder
# PLATFORM_DIRS trump all
ARTIFACTS = [
    # Source (windows)
    {
        'file': 'blast_source-%platform%@%version%',
        'platforms': ['windows'],
        'include': 
        [
            '*.h',
            '*.cpp',
            '*.inl',
            '*.capn',
            '/shared/external/hbao/**',
            '/shared/external/shadow_lib/**',
            '/docs/**',
            '/examples/**',
            '/images/**',
            'CMakeLists.txt',
            'license.txt',
            'README.md',
            '.gitignore',
            '*.cmake',
            '/*.xml',
            '/generate_projects_*win*.bat',
            '/download_sample_resources.bat',
            '/buildtools/build_platform_deps.xml',
            '/buildtools/get_build_deps.cmd',
            '/buildtools/cmake_projects_*win*.bat',
            '/buildtools/packman*/**',
            '/samples/resources/**',
        ],
        'exclude':
        [
            '**/CMakeFiles/**',
            '**/generated/*',
            'cmake_install.cmake',
            #'/test/**',
            '/shared/external/GraphicsLib/**',
            'SDK/source/waiting/**',
            'CMakeCXXCompilerId.*'
        ]
    },

    # Source (consoles)
    {
        'file': 'blast_source-%platform%@%version%',
        'platforms': ['xboxone', 'ps4', 'linux'],
        'include': 
        [
            '/sdk/**/*.h',
            '/sdk/**/*.cpp',
            '/sdk/**/*.inl',
            '/sdk/**/*.capn',
            '/sdk/**/CMakeLists.txt',
            '/sdk/**/*.cmake',
            '/test/**/*.h',
            '/test/**/*.cpp',
            '/test/**/CMakeLists.txt',
            '/test/**/*.cmake',
            '/test/**/*.png', #xboxone specific
            '/test/**/*.appxmanifest', #xboxone specific
            '/test/compiler/cmake/ps4/*.vcxproj.user', #ps4 specific, will be excluded by PLATFORM_DIRS on others
            '/shared/utils/*.h',
            '/shared/utils/*.cpp',
            '/docs/**',
            '/images/**',
            '/CMakeLists.txt',
            'license.txt',
            'README.md',
            '.gitignore',
            '/compiler/**/CMakeLists.txt',
            '/compiler/**/*.cmake',
            '/*.xml',
            '/generate_projects_%platform%.bat',
            '/generate_projects_%platform%.sh',
            '/buildtools/build_platform_deps.xml',
            '/buildtools/get_build_deps.cmd',
            '/buildtools/cmake_projects_%platform%.bat',
            '/buildtools/cmake_projects_%platform%.sh',
            '/buildtools/steps/build_all_linux.sh',
            '/buildtools/packman*/**',
			'/buildtools/platform/**' # PLATFORM_DIRS will only allow the proper subfolders
        ],
        'exclude':
        [
            '**/CMakeFiles/**',
            '**/generated/*',
            'cmake_install.cmake',
            'SDK/source/waiting/**',
            'CMakeCXXCompilerId.*'
        ]
    },

    # SDK Binary (all platforms)
    {
        'file': 'blast_sdk_binary-%platform%@%version%',
        'platforms': ['windows', 'xboxone', 'ps4', 'linux'],
        'include':
        [
            '/sdk/**/include/*.h',
            '/bin/*/*NvBlast*.dll',
            '/bin/*/*NvBlast*.so',
            '/lib/*/*NvBlast*.lib',
            '/lib/*/*NvBlast*.a',
            '/docs/api_docs/**',
            '/docs/release_notes.txt'
        ]
    },

    # Tools and Samples Binary (windows)
    {
        'file': 'blast_tools_and_samples-%platform%@%version%',
        'platforms': ['windows'],
        'include':
        [
            '/bin/vc14win64-cmake/*PROFILE*.dll',
            '/bin/vc14win64-cmake/*PROFILE*.exe',
            '/bin/vc14win64-cmake/nvToolsExt64_1.dll',
            '/bin/vc14win64-cmake/d3dcompiler_47.dll',
            '/bin/vc14win64-cmake/GFSDK_ShadowLib_DX11.win64.dll',
            '/bin/vc14win64-cmake/GFSDK_SSAO_D3D11.win64.dll',
            '/samples/resources/**',
            '/docs/api_docs/**',
            '/docs/release_notes.txt',
            '/buildtools/packman*/**',
            '/resources.xml',
            '/download_sample_resources.bat',
        ]
    },

]


def get_root_path():
    my_dir = os.path.dirname(os.path.realpath(__file__))
    path_prefix = os.path.abspath(os.path.join(my_dir, "../.."))
    return path_prefix


def iterable_artifacts(root_path, artifact_map):
    # We need to run formic with default_excludes disabled because otherwise .gitignore is filtered out (it is part
    # of default exclusion set in formic):
    fileset = formic.FileSet(directory=root_path, include=artifact_map['include'], exclude=artifact_map['exclude'],
                             default_excludes=False)
    for path in fileset:
        rel_path = os.path.relpath(path, root_path)
        temp_dir_path = None
        # We filter out GTL credentials from packages since they go to the public. Note that a perpetrator would need
        # to be inside NVIDIA network to use these credentials and would only be able to use them with packman
        # to get packages - so one could argue that this isn't important but better safe than sorry.
        if rel_path.endswith('packman_config.txt'):
            temp_dir_path = tempfile.mkdtemp()
            modified_file_path = os.path.join(temp_dir_path, 'packman_config.txt')
            with open(path, 'r') as file_in, open(modified_file_path, 'w') as file_out:
                lines_in = file_in.readlines()
                lines_out = []
                for line in lines_in:
                    if not line.startswith('PM_GTL'):
                        lines_out.append(line)
                file_out.writelines(lines_out)
            path = modified_file_path
        # need to handle packman 5 format as well and remove all gtl references:
        elif rel_path.endswith('config.packman.xml'):
            tree = ET.parse(path)
            root = tree.getroot()
            # remove gtl from 'remotes' cascade list
            remote_cascade = root.attrib['remotes'].split()
            filtered_list = []
            for item in remote_cascade:
                if item.lower() == 'gtl':
                    continue
                else:
                    filtered_list.append(item)
            root.attrib['remotes'] = ' '.join(filtered_list)
            # remove gtl remote configuration entry
            for remote in root.iter('remote'):
                if remote.attrib['type'].lower() == 'gtl':
                    root.remove(remote)
            temp_dir_path = tempfile.mkdtemp()
            modified_file_path = os.path.join(temp_dir_path, 'packman.config.xml')
            tree.write(modified_file_path)
            path = modified_file_path

        yield (path, rel_path)
        if temp_dir_path:
            shutil.rmtree(temp_dir_path)


def make_platform_string(string, platform):
    out = string.replace('%platform%', platform)
    #out = out.replace('/', os.path.sep)
    return out


def make_platform_dict(in_dict, platform):
    out_dict = {}
    for k, v in in_dict.iteritems():
        k_platform = make_platform_string(k, platform)
        if isinstance(v, dict):
            out_dict[k_platform] = make_platform_dict(v, platform)
        elif isinstance(v, list):
            v_platform = []
            out_dict[k_platform] = v_platform
            for item in v:
                item_platform = make_platform_string(item, platform)
                v_platform.append(item_platform)
        elif isinstance(v, str):
            out_dict[k_platform] = make_platform_string(v, platform)
    return out_dict


def main():
    parser = argparse.ArgumentParser(description='Create Blast packages for distribution.')
    parser.add_argument('platform', choices=PLATFORMS, help='platform to assemble packages for')
    parser.add_argument('-v', '--version', help='version to tag packages with')
    args = parser.parse_args()

    # Pack zip with some extension filter
    platform = args.platform
    for artifact in ARTIFACTS:
        if platform not in artifact['platforms']:
            continue
        version = "undefined"
        if args.version:
            version = args.version
        # make a copy and perform substitutions
        platform_artifact = make_platform_dict(artifact, platform)
        # extend the dict with illegal path prefixes for the current platform:
        illegal_paths = []
        for platform_name, paths in PLATFORM_DIRS.iteritems():
            if platform_name != platform:
                illegal_paths.extend(paths)

        platform_artifact.setdefault('exclude', [])
        platform_artifact['exclude'] += illegal_paths
        package_name = platform_artifact['file'].replace('%version%', version)
        root_path = get_root_path()
        package_path = os.path.join(root_path, package_name)

        artifacts_iterator = iterable_artifacts(root_path, platform_artifact)
        packager.create_package(package_path, artifacts_iterator)


if __name__ == "__main__":
    main()

