-- Shared build scripts from repo_build package
repo_build = require("omni/repo/build")

-- Repo root
root = repo_build.get_abs_path(".")

-- Insert kit template premake configuration, it creates solution, finds extensions.. Look inside for more details.
dofile("_repo/deps/repo_kit_tools/kit-template/premake5.lua")

-- Application example. Only runs Kit with a config, doesn't build anything. Helper for debugging.
define_app("omni.app.blast", { define_test = false })
