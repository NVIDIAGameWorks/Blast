newoption {
    trigger     = "platform-host",
    description = "(Optional) Specify host platform for cross-compilation"
}
newoption {
    trigger     = "python-version",
    description = "(Optional) Specify the python version to link against",
    allowed = {
        { "0",  "No Python" },
        { "27", "Python 2.7" },
        { "36", "Python 3.6" },
        { "37", "Python 3.7" }
    },
    default = "37"
}
local pylibs = {
    ["0"] = "",
    ["27"] = "python2.7",
    ["36"] = "python3.6m",
    ["37"] = "python3.7m",
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
local currentAbsPath = repo_build.get_abs_path(".");

-- premake5.lua
workspace "blast-sdk"
    configurations { "debug", "release" }
    startproject "test.unit"
    local targetName = _ACTION
    local workspaceDir = "_compiler/"..targetName
    -- common dir name to store platform specific files
    local platform = "%{cfg.system}-%{cfg.platform}"
    local targetDependencyPlatform = "%{cfg.system}-%{cfg.platform}";
    local hostDependencyPlatform = _OPTIONS["platform-host"] or targetDependencyPlatform;
    local targetDir = "_build/"..platform.."/%{cfg.buildcfg}"
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

    defines { "LOG_COMPONENT=\"%{prj.name}\"" }

    sysincludedirs { targetDepsDir }
    -- sysincludedirs { targetDepsDir.."/omni-trace/include" }
    -- sysincludedirs { targetDepsDir.."/omni-config-cpp/include" }

    -- link_curl()

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
        disablewarnings {
            "4100", -- unreferenced formal parameter
            "4127", -- conditional expression is constant
            -- "4189", -- 'x': local variable is initialized but not referenced
            "4201", -- nonstandard extension used: nameless struct/union
            -- "4456", -- declaration of 'x' hides previous local declaration (this happens a lot with OMNI_TRACE_SCOPE)
            -- "4506", -- no definition for inline function (this happens with Pixar headers)
        }
        -- defines { "_CRT_SECURE_NO_WARNINGS" }
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

function standard_blast_lib_setup(path)
    kind "SharedLib"
    location (workspaceDir.."/%{prj.name}")
    files {
        "%{root}/source/sdk/common/*.*",
        "%{root}/"..path.."/include/*.*",
        "%{root}/"..path.."/source/*.*",
    }

    filter { "system:windows" }
        -- defines { "ISOLATION_AWARE_ENABLED=1" }
    filter { "system:linux" }
        buildoptions { "-fPIC" }
        links { "rt" }
    filter{}

    includedirs {
        "%{root}/source/sdk/common",
        "%{root}/"..path.."/include",
        "%{root}/"..path.."/source",
    }

    vpaths {
        ["common/*"] = "%{root}/source/sdk/common",
        ["include/*"] = "%{root}/"..path.."/include/",
        ["source/*"] = "%{root}/"..path.."/source/",
    }
end

group "sdk"
    project "NvBlast"
        standard_blast_lib_setup("source/sdk/lowlevel")

    project "NvBlastGlobals"
        standard_blast_lib_setup("source/sdk/globals")
        includedirs {
            "%{root}/source/sdk/lowlevel/include",
        }

    project "NvBlastTk"
        dependson { "NvBlast", "NvBlastGlobals" }
        standard_blast_lib_setup("source/sdk/toolkit")
        includedirs {
            "%{root}/source/sdk/lowlevel/include",
            "%{root}/source/sdk/globals/include",
            "%{root}/source/sdk/globals/source",
            target_deps.."/physxsdk/include",
            target_deps.."/physxsdk/source/foundation/include",
            target_deps.."/pxshared/include",
        }
        libdirs { targetDir }
        links { "NvBlast.lib", "NvBlastGlobals.lib" }

    project "NvBlastExtAssetUtils"
        dependson { "NvBlast", "NvBlastGlobals" }
        standard_blast_lib_setup("source/sdk/extensions/assetutils")
        includedirs {
            "%{root}/source/sdk/lowlevel/include",
            "%{root}/source/sdk/globals/include",
        }
        libdirs { targetDir }
        links { "NvBlast.lib", "NvBlastGlobals.lib" }

    project "NvBlastExtAuthoring"
        dependson { "NvBlast", "NvBlastGlobals" }
        standard_blast_lib_setup("source/sdk/extensions/authoring")
        files {
            "%{root}/source/sdk/extensions/authoringCommon/include/*.*",
            "%{root}/source/sdk/extensions/authoringCommon/source/*.*",
            "%{root}/source/sdk/extensions/authoring/source/VHACD/inc/*.*",
            "%{root}/source/sdk/extensions/authoring/source/VHACD/src/*.*",
        }
        includedirs {
            "%{root}/source/sdk/lowlevel/include",
            "%{root}/source/sdk/globals/include",
            "%{root}/source/sdk/extensions/assetutils/include",
            "%{root}/source/sdk/extensions/authoringCommon/include",
            "%{root}/source/sdk/extensions/authoringCommon/source",
            "%{root}/source/sdk/extensions/authoring/source/VHACD/inc",
            "%{root}/source/sdk/extensions/authoring/source/VHACD/public",
            target_deps.."/physxsdk/include",
            target_deps.."/physxsdk/source/foundation/include",
            target_deps.."/pxshared/include",
            target_deps.."/BoostMultiprecision",
        }
        vpaths {
            ["VHACD/*"] = "%{root}/source/sdk/extensions/authoring/source/VHACD/",
            ["authoringCommon/*"] = "%{root}/source/sdk/extensions/authoringCommon/",
        }
        disablewarnings {
            "4244", -- conversion from 'type1' to 'type2', possible loss of data
            "4267", -- conversion from 'size_t' to 'type', possible loss of data
        }
        libdirs { targetDir }
        links { "NvBlast.lib", "NvBlastGlobals.lib" }

    -- project "NvBlastExtExporter"
    -- project "NvBlastExtPhysX"
    -- project "NvBlastExtPxSerialization"
    -- project "NvBlastExtSerialization"
    -- project "NvBlastExtShaders"
    -- project "NvBlastExtStress"
    -- project "NvBlastExtTkSerialization"
