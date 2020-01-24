import os
import sys
import argparse

import repoman

repoman.bootstrap()

import omni.repo.man
import omni.repo.package


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_ROOT_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, "..", ".."))
VERSION_PATH = os.path.realpath(os.path.join(REPO_ROOT_DIR, "version"))

def get_version():
    with open(VERSION_PATH, 'r') as file:
        version = file.read().replace('\n', '')
    if version is None:
        if os.getenv("BUILD_NUMBER"):
            version = os.getenv("BUILD_NUMBER")
        else:
            version = None
    return version


def create_package_desc(platform_target: str, config: str) -> omni.repo.package.PackageDesc:
    package = omni.repo.package.PackageDesc()
    package.version = get_version()
    package.append_git_hash = False

    # For local package add branch and version
    if package.version is None:
        package.append_git_hash = True
        package.append_git_branch = True

    package.name = f"blastsdk"
    package.append_platform = False
    package.custom_platform = platform_target
    package.ziponly = False
    package.output_folder = "_builtpackages"
    package.remove_pycache = True
    package.warn_if_not_exist = True
    return package


def create_gfx_package_desc(platform_target: str, config: str) -> omni.repo.package.PackageDesc:
    package = create_package_desc(platform_target, config)
    package.name = "carb_gfx_plugins"
    package.version = None
    package.ziponly = False
    package.append_git_hash = False
    package.output_folder = "_builtpackages"
    package.remove_pycache = True

    package.version = os.getenv("BUILD_NUMBER")
    if not package.version:
        package.version = "0"

    package.label_name = "%s@%s-%s.latest.txt" % (
        package.name,
        package.version[: package.version.find(".")],
        platform_target,
    )
    return package


def create_rtx_package_desc(platform_target: str, config: str) -> omni.repo.package.PackageDesc:
    package = create_package_desc(platform_target, config)
    package.name = "rtx_plugins"
    package.version = None
    package.ziponly = False
    package.append_git_hash = False
    package.output_folder = "_builtpackages"

    package.version = os.getenv("BUILD_NUMBER")
    if not package.version:
        package.version = "0"

    package.label_name = "%s@%s-%s.latest.txt" % (
        package.name,
        package.version[: package.version.find(".")],
        platform_target,
    )
    return package


def create_blastsdk_package_desc(platform_target: str, config: str) -> omni.repo.package.PackageDesc:
    package = create_package_desc(platform_target, config)
    package.build_type = config
    package.compression_level_7z = 1
    return package


def create_symbols_package_desc(platform_target: str, config: str) -> omni.repo.package.PackageDesc:
    if platform_target == "windows-x86_64":
        package = create_package_desc(platform_target, config)
        package.name = "release_symbols"
        package.version = None
        package.ziponly = True
        package.append_git_hash = False
        return package
    return None


def create_testrunner_package_desc(platform_target: str, config) -> omni.repo.package.PackageDesc:
    package = create_package_desc(platform_target, config)
    package.name = "test_runner"
    package.version = None
    package.ziponly = True
    package.append_git_hash = False
    package.files = ["deps", "tools"]
    return package


PACKAGES = {
    "blastsdk": create_blastsdk_package_desc,
}

CONFIGS = ["release", "debug", "checked", "profile"]
PLATFORMS = ["windows-x86_64", "linux-x86_64"]


def run_command():
    platform_host = omni.repo.man.get_and_validate_host_platform(["windows-x86_64", "linux-x86_64"])

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument(
        "-p",
        "--platform-target",
        dest="platform_target",
        default=platform_host,
        choices=PLATFORMS,
        help="Platform Target",
    )
    parser.add_argument("-c", "--config", dest="config", default=CONFIGS[0], choices=CONFIGS, help="Platform config.")
    parser.add_argument(
        "-m",
        "--package-mode",
        dest="package",
        choices=PACKAGES.keys(),
        default="blastsdk",
        help="Package to create.",
    )

    options = parser.parse_args()

    # Install toml and read package.toml
    repo_folders = omni.repo.man.get_repo_paths()
    omni.repo.man.pip_install("toml", repo_folders["pip_packages"])
    import toml

    package_dict = toml.load(os.path.join(repo_folders["root"], "package.toml"))

    # Prepare package desc and package
    package_desc = PACKAGES[options.package](options.platform_target, options.config)
    if package_desc is None:
        print("Error: Nothing to package for this configuration.")
        sys.exit(-1)

    package_desc.files = omni.repo.man.gather_files_from_dict_for_platform(
        package_dict, options.package, options.platform_target, [options.config]
    )
    omni.repo.package.package(package_desc)


if __name__ == "__main__":
    run_command()
