import os
import argparse
import glob
import packmanapi


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
ROOT_DIR = os.path.realpath(os.path.join(SCRIPT_DIR, "..", ".."))


def get_packages_and_labels(package_name, config):
    # we have to upload the packages before the labels. so we generate 2 different lists and process in order.
    packages = []
    labels = []

    package_mask = f"{package_name}@*"
    if config:
        package_mask = package_mask + f"-{config}*"
    for filename in glob.glob(f"{ROOT_DIR}/_builtpackages/{package_mask}"):
        _, ext = os.path.splitext(filename)
        if ext in [".7z", ".zip"]:
            packages.append(filename)
        elif ext in [".txt"]:
            labels.append(filename)
    return packages, labels


PACKAGES = ["blastsdk"]


def main():
    package_version = None

    parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-p", "--package", dest="package", choices=PACKAGES, default="blastsdk", help="Package.")
    parser.add_argument("-f", "--force", dest="force", default=False, action="store_true", help="Force publish.")
    parser.add_argument(
        "-c",
        "--config",
        dest="config",
        required=False,
        help="Config to choose (for packages with debug/release split). (default: %(default)s)",
    )
    parser.add_argument(
        "-r",
        "--remote",
        dest="remote",
        required=False,
        default="packman:cloudfront_upload",
        help="Packman remote to use.",
    )
    options = parser.parse_args()

    packages, labels = get_packages_and_labels(options.package, options.config)

    for package in packages:
        if not package_version:
            package_version = "-".join(package.split("-", 2)[:2]).split("@")[1]
        print("Publishing Package %s" % package)
        try:
            packmanapi.push(package, remotes=[options.remote], force=options.force)
        except packmanapi.PackmanErrorFileExists:
            print("\n\nThe package '%s' already exists on the remote server." % package)
            print("The process will be aborted and requires a new build to continue.\n")
            print("If the process continued it could update existing packages with changes or if newer packages")
            print("exist, it could change the labels to point to an older versions of the packages.\n")
            print("IF you want to force the upload of the packages please rerun with the arg '--force'\n\n")
            if os.getenv("TEAMCITY_VERSION"):  # check if its teamcity else there is no point printing this message.
                if package_version:
                    print(
                        "##teamcity[buildStatus status='success' text='Package version %s already deployed.']"
                        % package_version
                    )
            return
    for label in labels:
        print("Publishing Label %s" % label)
        packmanapi.push(label, remotes=[options.remote], force=True)
    if os.getenv("TEAMCITY_VERSION"):  # check if its teamcity else there is no point printing this message.
        if package_version:
            print("##teamcity[buildStatus status='success' text='Package version %s deployed.']" % package_version)


if __name__ == "__main__":
    main()
