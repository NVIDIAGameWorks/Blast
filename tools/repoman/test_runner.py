import os
import sys
import glob
import shutil
import logging
import argparse
import base64
import hashlib
from typing import List

import packmanapi
import repoman

repoman.bootstrap()
import omni.repo.man

logger = logging.getLogger(os.path.basename(__file__))

ARCHIVE_PATTERN = "_builtpackages/blastsdk*-{config}.7z"


def is_running_under_teamcity():
    return bool(os.getenv("TEAMCITY_VERSION"))


def short_hash(name: str, length: int = 5) -> str:
    hasher = hashlib.sha1(name.encode("utf-8"))
    return base64.urlsafe_b64encode(hasher.digest()[:length]).decode("ascii").rstrip("=")


def get_exe_ext(platform: str) -> str:
    return ".exe" if platform == "windows-x86_64" else ""


def prepare_package(root: str, config: str, clean: bool) -> str:
    """Find and extract a package, return path to a folder"""

    candidates = list(glob.glob(os.path.join(root, ARCHIVE_PATTERN.format(config=config))))
    if len(candidates) == 0:
        logger.error(f"No archive files found.")
        sys.exit(-1)

    archive_path = candidates[0]
    if len(candidates) > 1:
        logger.warn(f"Multiple candidates for archive file, selecting first: {archive_path}")

    if not os.path.exists(archive_path):
        logger.error(f"Archive file doesn't exist: {archive_path}")
        sys.exit(-1)

    # Shorten folder name to workaround "too long path issue on TC"
    filename, _ = os.path.splitext(os.path.basename(archive_path))
    folder_to_extract = os.path.join(os.path.dirname(archive_path), short_hash(filename))

    if clean:
        if os.path.exists(folder_to_extract):
            logger.info(f"Cleaning folder: {folder_to_extract}")
            shutil.rmtree(folder_to_extract)

    if not os.path.exists(folder_to_extract):
        packmanapi.extract_archive7z_to_folder(archive_path, folder_to_extract)

    return folder_to_extract


def run_unittests(root: str, platform_host: str, config: str, extra_args: List = []):
    executable = f"test.unit{get_exe_ext(platform_host)}"

    args = []
    if is_running_under_teamcity():
        args.append("-r teamcity")
    args.extend(extra_args)

    omni.repo.man.run_process(
        [f"{root}/_build/{platform_host}/{config}/plugins/{executable}"] + args, exit_on_error=True
    )


def run_pythontests(root: str, platform_host: str, config: str, extra_args: List = []):
    """Run python tests suite inside of Kit"""

    executable = f"blastsdk{get_exe_ext(platform_host)}"
    args = ["--exec", '"run_tests.py"']
    args.extend(extra_args)
    omni.repo.man.run_process([f"{root}/_build/{platform_host}/{config}/{executable}"] + args, exit_on_error=True)


def run_startuptest(root: str, platform_host: str, config: str, extra_args: List = []):
    """Start and quit Kit"""

    executable = f"blastsdk{get_exe_ext(platform_host)}"
    args = ["--exec", "quit"]
    args.extend(extra_args)
    omni.repo.man.run_process([f"{root}/_build/{platform_host}/{config}/{executable}"] + args, exit_on_error=True)


def run_qatests(root: str, platform_host: str, config: str, extra_args: List = []):
    executable = f"blastsdk{get_exe_ext(platform_host)}"

    for runid in range(1, 5):
        args = ["--exec", f"unit_test.py -runid {runid}", "--carb/app/omniverse/showLoginOnStart=false"]
        args.extend(extra_args)
        omni.repo.man.run_process([f"{root}/_build/{platform_host}/{config}/{executable}"] + args, exit_on_error=True)


TEST_SUITES = {
    "unittests": run_unittests,
    "pythontests": run_pythontests,
    "startuptest": run_startuptest,
    "qatests": run_qatests,
}


def main():
    platform_host = omni.repo.man.get_and_validate_host_platform(["windows-x86_64", "linux-x86_64"])
    repo_folders = omni.repo.man.get_repo_paths()

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.name = "Universal Test Runner"
    parser.add_argument(
        "-p",
        "--from-package",
        dest="from_package",
        default=False,
        action="store_true",
        help=f"Use package from '{ARCHIVE_PATTERN}' instead of a root folder.",
    )
    parser.add_argument(
        "-x",
        "--clean",
        dest="clean",
        default=False,
        action="store_true",
        help="Clean run (force extract package again).",
    )
    parser.add_argument(
        "--suite", dest="suite", choices=TEST_SUITES.keys(), default="unittests", help="Test suite to run."
    )
    parser.add_argument(
        "-c",
        "--config",
        dest="config",
        required=False,
        default="debug",
        help="Config to run test against (debug or release). (default: %(default)s)",
    )
    parser.add_argument(
        "-e",
        "--extra-arg",
        action="append",
        dest="extra_args",
        default=[],
        help="Extra argument to pass. Can be specified multiple times.",
    )

    options = parser.parse_args()

    root_folder = repo_folders["root"]
    if options.from_package:
        root_folder = prepare_package(root_folder, options.config, options.clean)

    logger.info(f"Running test suite: {options.suite}...")
    TEST_SUITES[options.suite](root_folder, platform_host, options.config, options.extra_args)


if __name__ == "__main__":
    main()
