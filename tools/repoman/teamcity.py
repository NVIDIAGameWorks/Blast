import os
import argparse

import repoman

repoman.bootstrap()
import omni.repo.man


MAJOR_VERSION = 3


def run_command():
    parser = argparse.ArgumentParser()
    parser.add_argument("--prepbuild", dest="prepbuild", action="store_true")
    parser.add_argument("--buildnumber", dest="buildnumber", action="store_true")
    options = parser.parse_args()

    build_number = os.getenv("BUILD_NUMBER")
    if not build_number:
        build_number = "0"

    gitbranch = os.getenv("buildbranch")
    if not gitbranch:
        gitbranch = omni.repo.man.call_git(["rev-parse", "--abbrev-ref", "HEAD"]).decode("utf8")

    githash = omni.repo.man.call_git(["rev-parse", "--short=8", "HEAD"]).decode("utf8")

    final_build_number = f"2019.{MAJOR_VERSION}.{build_number}.{githash}"
    if "master" not in gitbranch.lower() and "/release/" not in gitbranch.lower():
        if len(gitbranch.split("/")) > 1:
            merge_request_number = gitbranch.split("/")[1]
            final_build_number = f"{final_build_number}-mr{merge_request_number}"
        else:
            final_build_number = f"{final_build_number}-{gitbranch}"

    # Preparing and early exitting the TeamCity build
    if options.prepbuild:
        print(f"##teamcity[setParameter name='build.number' value='{final_build_number}']")
        print(f"##teamcity[buildNumber '{final_build_number}']")

    if options.buildnumber:
        print(final_build_number)


if __name__ == "__main__":
    run_command()
