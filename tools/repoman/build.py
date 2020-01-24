import os
import sys
import platform

import logging


import packmanapi


logger = logging.getLogger(os.path.basename(__file__))


# VSCode python environment variables
VSCODE_PYTHON_ENV = {
    "PYTHONPATH": [
        "$${PYTHONPATH}",
        os.environ.get("PYTHONPATH"),
        "$root/_build/pip-packages",
        "$repo_deps/repo_man",
        "$repo_deps/repo_build",
        "$repo_deps/repo_fileutils",
        "$repo_deps/repo_format",
        "$repo_deps/repo_package",
        "$repo_deps/repo_licensing",
        "$root/tools/repoman",
        "$root/_build/$platform/$config/extensions/extensions-bundled",
        "$root/_build/$platform/$config/extensions/extensions-other",
        "$root/_build/$platform/$config/plugins/bindings-python",
    ],
    "PATH": ["$${PATH}", "$root/_build/$platform/$config", "$root/_build/$platform/$config/plugins"],
}


def clean():
    logger.info("cleaning repo:")
    folders = ["_build", "_compiler", "_builtpackages"]

    root = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..")
    for folder in folders:
        folder = os.path.abspath(os.path.join(root, folder))
        # having to do the platform check because its safer when you might be removing
        # folders with windows junctions.
        if os.path.exists(folder):
            logger.info(f"removing folder: {folder}")
            if platform.system() == "Windows":
                os.system("rmdir /q /s %s > nul 2>&1" % folder)
            else:
                os.system("rm -r -f %s > /dev/null 2>&1" % folder)
            if os.path.exists(folder):
                logger.warn(
                    f"{folder} was not successfully removed, most probably due to a file lock on 1 or more of the files."
                )


def pull_dependencies_custom(repo_folders, platform_host, platform_target):
    """ Custom pull dependencies function."""
    from omni.repo.build import pull_dependencies

    # Call default pull
    pull_dependencies(repo_folders, platform_host, platform_target)

    # Add more deps:
    packmanapi.pull(os.path.join(repo_folders["deps_xml_folder"], "resources.packman.xml"), platform=platform_target)
    packmanapi.pull(os.path.join(repo_folders["deps_xml_folder"], "library.packman.xml"), platform=platform_target)


def main():
    import repoman

    repoman.configure_logging()

    # Cleaning is done before loading repo_build too to avoid chicken and egg problem (repo_build deletes itself).
    if any(flag in sys.argv for flag in ["-c", "--clean"]):
        clean()
        return
    if any(flag in sys.argv for flag in ["-x", "--rebuild"]):
        clean()

    # Load repoman only now to not mess with clean step which will remove host-deps folder.
    repoman.bootstrap()
    import omni.repo.man
    import omni.repo.build

    # Setup and call into repo_build tool
    root = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "..")
    settings = omni.repo.build.Settings()
    settings.stage_files_extra_mapping = {"carb_sdk": os.path.join(root, "_build/target-deps/carb_sdk_plugins")}
    settings.stage_files_error_if_missing = False
    settings.vscode_python_env = VSCODE_PYTHON_ENV
    settings.sln_file = "OmniverseKit.sln"
    settings.vs_version = "vs2017"
    omni.repo.build.main(root=root, settings=settings, pull_dependencies_fn=pull_dependencies_custom)


if __name__ == "__main__":
    main()
