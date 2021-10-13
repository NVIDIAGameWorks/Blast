newoption {
    trigger     = "platform-host",
    description = "(Optional) Specify host platform for cross-compilation"
}

-- Include omni.repo.build premake tools
local repo_build = require('omni/repo/build')

-- Path defines
local target_deps = "%{root}/_build/target-deps"

-- Enable /sourcelink flag for VS
repo_build.enable_vstudio_sourcelink()

-- Remove /JMC parameter for visual studio
repo_build.remove_vstudio_jmc()

-- Wrapper funcion around path.getabsolute() which makes drive letter lowercase on windows.
-- Otherwise drive letter can alter the case depending on environment and cause solution to reload.
function get_abs_path(p)
    p = path.getabsolute(p)
    if os.target() == "windows" then
        p = p:gsub("^%a:", function(c) return c:lower() end)
    end
    return p
end

function copy_to_file(filePath, newPath)
    local filePathAbs = get_abs_path(filePath)
    local dir = newPath:match("(.*[\\/])")
    if os.target() == "windows" then
        if dir ~= "" then
            --dir = dir:gsub('/', '\\')
            postbuildcommands { "{MKDIR} \""..dir.."\"" }
        end
        -- Using {COPY} on Windows adds an IF EXIST with an extra backslash which doesn't work
        filePathAbs = filePathAbs:gsub('/', '\\')
        newPath = newPath:gsub('/', '\\')
        postbuildcommands { "copy /Y \""..filePathAbs.."\" \""..newPath.."\"" }
    else
        if dir ~= "" then
            postbuildcommands { "$(SILENT) {MKDIR} "..dir }
        end
        postbuildcommands { "$(SILENT) {COPY} "..filePathAbs.." "..newPath }
    end
end

function os.capture(cmd, raw)
  local f = assert(io.popen(cmd, 'r'))
  local s = assert(f:read('*a'))
  f:close()
  if raw then return s end
  s = string.gsub(s, '^%s+', '')
  s = string.gsub(s, '%s+$', '')
  s = string.gsub(s, '[\n\r]+', ' ')
  return s
end

premake.override(premake.vstudio.vc2010, "projectReferences", function(base, prj)
   local refs = premake.project.getdependencies(prj, 'linkOnly')
   if #refs > 0 then
      premake.push('<ItemGroup>')
      for _, ref in ipairs(refs) do
         local relpath = premake.vstudio.path(prj, premake.vstudio.projectfile(ref))
         premake.push('<ProjectReference Include=\"%s\">', relpath)
         premake.callArray(premake.vstudio.vc2010.elements.projectReferences, prj, ref)
         premake.vstudio.vc2010.element("UseLibraryDependencyInputs", nil, "true")
         premake.pop('</ProjectReference>')
      end
      premake.pop('</ItemGroup>')
   end
end)

local hostDepsDir = "_build/host-deps"
local targetDepsDir = "_build/target-deps"
local root = repo_build.get_abs_path(".");

-- Would be nice to be able to use this to define the actual workspace name
local workspace_name = "blast-sdk"

-- Copy headers and licenses
repo_build.prebuild_copy {
    { "%{root}/include", "%{root}/_build/%{platform}/%{config}/"..workspace_name.."/include" },
    { "%{root}/source/sdk/common", "%{root}/_build/%{platform}/%{config}/"..workspace_name.."/source/sdk/common" },
    { "%{root}/PACKAGE-LICENSES", "%{root}/_build/%{platform}/%{config}/"..workspace_name.."/PACKAGE-LICENSES" }
}

-- premake5.lua
workspace "blast-sdk"
    configurations { "debug", "release" }
    startproject "NvBlast"
    local targetName = _ACTION
    local workspaceDir = "_compiler/"..targetName
    -- common dir name to store platform specific files
    local platform = "%{cfg.system}-%{cfg.platform}"
    local targetDependencyPlatform = "%{cfg.system}-%{cfg.platform}";
    local hostDependencyPlatform = _OPTIONS["platform-host"] or targetDependencyPlatform;
    local sdkTargetDir = "_build/"..platform.."/%{cfg.buildcfg}/%{wks.name}"
    local targetDir = sdkTargetDir.."/bin"
    -- defining anything related to the VS or SDK version here because they will most likely be changed in the future..
    local msvcInclude = hostDepsDir.."/msvc/VC/Tools/MSVC/14.16.27023/include"
    local msvcLibs = hostDepsDir.."/msvc/VC/Tools/MSVC/14.16.27023/lib/onecore/x64"
    local sdkInclude = { hostDepsDir.."/winsdk/include/winrt", hostDepsDir.."/winsdk/include/um", hostDepsDir.."/winsdk/include/ucrt", hostDepsDir.."/winsdk/include/shared" }
    local sdkLibs = { hostDepsDir.."/winsdk/lib/ucrt/x64", hostDepsDir.."/winsdk/lib/um/x64" }

    location (workspaceDir)
    targetdir (targetDir)
    -- symbolspath ("_build/"..targetName.."/symbols/%{cfg_buildcfg}/%{prj.name}.pdb")
    objdir ("_build/tmp/%{cfg.system}/%{prj.name}")
    exceptionhandling "On"
    rtti "Off"
    staticruntime "Off"
    flags { "FatalCompileWarnings", "MultiProcessorCompile", "NoPCH", "UndefinedIdentifiers", "NoIncrementalLink" }
    cppdialect "C++14"
    includedirs { "include" }

    characterset( "ASCII" )

    defines { "LOG_COMPONENT=\"%{prj.name}\"" }

    sysincludedirs { targetDepsDir }

    filter { "system:windows" }
        platforms { "x86_64" }
        symbols "Full"
        -- add .editorconfig to all projects so that VS 2017 automatically picks it up
        files {".editorconfig"}
        editandcontinue "Off"
        bindirs { hostDepsDir.."/msvc/VC/Tools/MSVC/14.16.27023/bin/HostX64/x64", hostDepsDir.."/msvc/MSBuild/15.0/bin", hostDepsDir.."/winsdk/bin/x64" }
        systemversion "10.0.17763.0"
        -- this is for the include and libs from the SDK.
        syslibdirs { msvcLibs, sdkLibs }
        sysincludedirs { msvcInclude, sdkInclude }
        -- all of our source strings and executable strings are utf8
        buildoptions {"/utf-8", "/bigobj"}
        buildoptions {"/permissive-"}
        buildoptions { "/WX" } -- warnings as errors
        warnings "Extra"
        defines { "_CRT_NONSTDC_NO_DEPRECATE" }
        defines { "BOOST_USE_WINDOWS_H=1" }

    filter { "system:linux" }
        platforms { "x86_64", "aarch64" }
        defaultplatform "x86_64"
    filter { "system:linux", "platforms:x86_64" }
        defines { "_GLIBCXX_USE_CXX11_ABI=0" }
        architecture "x86_64"
    filter { "system:linux", "platforms:aarch64" }
        defines { "_GLIBCXX_USE_CXX11_ABI=1" }
        architecture "ARM"
        local hostDependencyPlatform = _OPTIONS["platform-host"]
        print(hostDependencyPlatform)
        -- If cross-compiling, set the toolset explicitly
        if (localDependencyPlatform and localDependencyPlatform == "linux-x86_64") then
            local toolchain_path = "_build/host-deps/gcc-x86_64"
            local toolchain = dofile(toolchain_path .. "/toolchain.lua")
            use_gcc_local_toolchain(toolchain_path)
            toolset("gcc-local_9_2_0_arch64")
        end
    filter { "system:linux" }
        defines { "__STDC_FORMAT_MACROS" }
        symbols "On"

        buildoptions { "-pthread -fvisibility=hidden -fnon-call-exceptions -D_FILE_OFFSET_BITS=64 -fabi-version=8" }
        -- enforces RPATH instead of RUNPATH, needed for ubuntu version > 16.04
        linkoptions { "-pthread", 
                      "-Wl,--no-undefined",
                      "-Wl,--disable-new-dtags",
                      --"-Wl,--version-script=../../../omniclient.so.ld",
                      "-Wl,-rpath,'$$ORIGIN' -Wl,--export-dynamic" }
        -- add library origin directory to dlopen() search path
        enablewarnings { "all", "vla" }
        disablewarnings {
            "unused-variable",
            "switch",
            "unused-but-set-variable",
            "unused-result",
            "deprecated",
            "deprecated-declarations",
            "unknown-pragmas",
            "multichar"
        }
        links { "stdc++fs" }
        if repo_build.ccache_path() then
            gccprefix (repo_build.ccache_path().." ")
        end
    filter {}

    filter { "system:linux", "configurations:debug" }
        buildoptions { "-ggdb", "-g3" }
    filter { "system:linux", "configurations:release" }
        buildoptions { "-ggdb", "-g2" }

    filter { "configurations:debug" }
        optimize "Off"
        defines { "CARB_DEBUG=1" }
    filter  { "configurations:release" }
        defines { "NDEBUG", "CARB_DEBUG=0" }
    filter  { "configurations:release", "system:windows" }
        optimize "Speed"
    -- Linux/GCC has some issues on thread exit when the "Speed" optimizations are enabled.
    -- We'll leave those off on Linux for the moment.
    filter { "configurations:release", "system:linux" }
        optimize "On"
    filter {}

function blast_sdklib_bare_setup(name)
    kind "SharedLib"
    location (workspaceDir.."/%{prj.name}")

    filter { "system:windows" }
        -- defines { "ISOLATION_AWARE_ENABLED=1" }
    filter { "system:linux" }
        buildoptions { "-fPIC" }
        links { "rt" }
    filter{}

    includedirs {
        "%{root}/source/sdk/common",
        "%{root}/include/"..name,
        "%{root}/source/sdk"..name,
    }
end

function blast_sdklib_common_files()
    files {
        "%{root}/source/sdk/common/*.cpp",
    }

    vpaths {
        ["common/*"] = "%{root}/source/sdk/common",
    }
end

function blast_sdklib_standard_setup(name)
    blast_sdklib_bare_setup(name)
    blast_sdklib_common_files()

    files {
        "%{root}/source/sdk/"..name.."/*.cpp",
    }

    vpaths {
        ["include/*"] = "%{root}/include/"..name,
        ["source/*"] = "%{root}/source/sdk/"..name.."/",
    }
end

function link_dependents(...)
    libdirs { targetDir }
    for i = 1, select('#', ...) do
        local project_name = select(i, ...)
        dependson(project_name)
        filter { "system:windows" }
            links(project_name..".lib")
        filter { "system:linux" }
            links("lib"..project_name)
        filter {}
    end
end

function add_files(rootpath, ...)
    for i = 1, select('#', ...) do
        local filename = select(i, ...)
        files { "%{root}/"..rootpath.."/"..filename }
    end
end

group "sdk"
    project "NvBlast"
        blast_sdklib_standard_setup("lowlevel")

    project "NvBlastGlobals"
        blast_sdklib_standard_setup("globals")
        includedirs {
            "%{root}/include/lowlevel",
        }

    project "NvBlastExtShaders"
        link_dependents("NvBlast", "NvBlastGlobals")
        blast_sdklib_standard_setup("extensions/shaders")
        includedirs {
            "%{root}/include/lowlevel",
            "%{root}/include/globals",
            target_deps.."/physxsdk/include",
            target_deps.."/physxsdk/source/foundation/include",
            target_deps.."/pxshared/include",
        }
        disablewarnings {
            "4267", -- conversion from 'size_t' to 'type', possible loss of data
        }

    project "NvBlastExtAssetUtils"
        link_dependents("NvBlast", "NvBlastGlobals")
        blast_sdklib_standard_setup("extensions/assetutils")
        includedirs {
            "%{root}/include/lowlevel",
            "%{root}/include/globals",
        }

        project "NvBlastExtAuthoring"
        link_dependents("NvBlast", "NvBlastGlobals")
        blast_sdklib_standard_setup("extensions/authoring")
        includedirs {
            "%{root}/include/lowlevel",
            "%{root}/include/globals",
            "%{root}/include/extensions/assetutils",
            "%{root}/include/extensions/authoringCommon",
            "%{root}/source/sdk/extensions/authoring",
            "%{root}/source/sdk/extensions/authoringCommon",
            "%{root}/source/sdk/extensions/authoring/VHACD/inc",
            "%{root}/source/sdk/extensions/authoring/VHACD/public",
            target_deps.."/physxsdk/include",
            target_deps.."/physxsdk/source/foundation/include",
            target_deps.."/pxshared/include",
            target_deps.."/BoostMultiprecision",
        }
        files {
            "%{root}/source/sdk/extensions/authoringCommon/*.cpp",
            "%{root}/source/sdk/extensions/authoring/VHACD/src/*.cpp",
        }
        vpaths {
            ["VHACD/*"] = "%{root}/source/sdk/extensions/authoring/VHACD/",
            ["authoringCommon/include/*"] = "%{root}/include/extensions/authoringCommon/",
            ["authoringCommon/source/*"] = "%{root}/source/sdk/extensions/authoringCommon/",
        }
        disablewarnings {
            "4244", -- conversion from 'type1' to 'type2', possible loss of data
            "4267", -- conversion from 'size_t' to 'type', possible loss of data
        }

    project "NvBlastExtSerialization"
        link_dependents("NvBlast", "NvBlastGlobals")
        blast_sdklib_bare_setup("extensions/serialization")
        local capnp_gen_path = "_build/generated_capnp"
        local capnp_gen_fullpath = "%{root}"..capnp_gen_path
        includedirs {
            "%{root}/source/sdk/extensions/serialization/DTO",
            "%{root}/include/lowlevel",
            "%{root}/source/sdk/lowlevel",
            "%{root}/include/globals",
            "%{root}/_build/host-deps/CapnProto/src",
            capnp_gen_fullpath,
            target_deps.."/physxsdk/include",
            target_deps.."/physxsdk/source/foundation/include",
            target_deps.."/pxshared/include",
        }
        blast_sdklib_common_files()
        add_files("_build/host-deps/CapnProto/src/capnp",
            "arena.c++",
            "blob.c++",
            "layout.c++",
            "message.c++",
            "serialize.c++"
        )
        add_files("_build/host-deps/CapnProto/src/kj",
            "array.c++",
            "common.c++",
            "debug.c++",
            "exception.c++",
            "io.c++",
            "mutex.c++",
            "string.c++",
            "units.c++"
        )
        add_files("source/sdk/extensions/serialization",
            "NvBlastExtSerialization.cpp",
            "NvBlastExtLlSerialization.cpp",
            "NvBlastExtOutputStream.cpp",
            "NvBlastExtInputStream.cpp"
        )
        add_files("source/sdk/extensions/serialization/DTO",
            "ActorDTO.cpp",
            "AssetDTO.cpp",
            "FamilyDTO.cpp",
            "FamilyGraphDTO.cpp",
            "NvBlastChunkDTO.cpp",
            "NvBlastBondDTO.cpp",
            "NvBlastIDDTO.cpp"
        )
        add_files(capnp_gen_path,
            "NvBlastExtLlSerialization.capn.c++"
        )
        vpaths {
            ["include/*"] = "%{root}/include/extensions/serialization/",
            ["source/*"] = "%{root}/source/sdk/extensions/serialization/",
        }

        -- cap'n proto precompile step
        local capnp_bin = get_abs_path("%{root}_build/host-deps/CapnProto/tools/win32")
        local capnp_gen = get_abs_path(capnp_gen_fullpath)
        filter { "system:windows" }
            capnp_bin = capnp_bin:gsub('/', '\\')
            capnp_gen = capnp_gen:gsub('/', '\\')
            prebuildcommands { "set PATH="..capnp_bin..";%PATH%" } -- set cap'n proto executable path
            prebuildcommands { "if not exist "..capnp_gen.."\\ mkdir "..capnp_gen } -- make the generated source folder under _build
            prebuildcommands { "pushd "..capnp_gen } -- push current path and go into the generated source folder
            prebuildcommands { "del /S *" } -- clear the generated source folder
            -- capnp compile
            prebuildcommands { "capnp compile -oc++ -I ../host-deps/CapnProto/src --src-prefix=../../source/sdk/extensions/serialization/ ../../source/sdk/extensions/serialization/NvBlastExtLlSerialization.capn" }
            prebuildcommands { "popd" } -- return to previous folder

            -- cap'n proto source produces a lot of warnings
            disablewarnings {
                "4018", -- 'token' : signed/unsigned mismatch
                "4100", -- unreferenced formal parameter
                "4189", -- 'identifier' : local variable is initialized but not referenced
                "4244", -- conversion from 'type1' to 'type2', possible loss of data
                "4245", -- conversion from 'type1' to 'type2', signed/unsigned mismatch
                "4267", -- conversion from 'size_t' to 'type', possible loss of data
                "4456", -- declaration of 'identifier' hides previous local declaration
                "4541", -- 'identifier' used on polymorphic type 'type' with /GR-; unpredictable behavior may result
                "4702", -- unreachable code
                "4714", -- function 'function' marked as __forceinline not inlined
            }
        filter {}

    project "NvBlastTk"
        link_dependents("NvBlast", "NvBlastGlobals")
        blast_sdklib_standard_setup("toolkit")
        includedirs {
            "%{root}/include/lowlevel",
            "%{root}/include/globals",
            "%{root}/source/sdk/globals",
            target_deps.."/physxsdk/include",
            target_deps.."/physxsdk/source/foundation/include",
            target_deps.."/pxshared/include",
        }

    -- project "NvBlastExtTkSerialization"
    -- project "NvBlastExtExporter"
    -- project "NvBlastExtStress"
    -- project "NvBlastExtPhysX"
    -- project "NvBlastExtPxSerialization"
