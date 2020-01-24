import os
import sys
import logging
import packmanapi

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
REPO_DEPS_FILE = os.path.join(SCRIPT_DIR, "../../deps/repo-deps.packman.xml")


def configure_logging():
    """
    Configure default logging.
    """

    # Replace current default handler with stdout handler and setup formatting
    root_logger = logging.getLogger()
    root_logger.handlers = []

    handler = logging.StreamHandler(sys.stdout)
    handler.setFormatter(logging.Formatter("[%(levelname)s][%(name)s] %(message)s"))
    root_logger.addHandler(handler)

    # Set log levels
    root_logger.setLevel(logging.INFO)
    logging.getLogger("botocore").setLevel(logging.WARN)
    logging.getLogger("packman").setLevel(logging.WARN)
    logging.getLogger("omni.repo").setLevel(logging.INFO)


def bootstrap():
    """
    Bootstrap all omni.repo modules.

    Pull with packman from repo.packman.xml and add them all to python sys.path to enable importing.
    """
    configure_logging()

    deps = packmanapi.pull(REPO_DEPS_FILE)
    for dep_path in deps.values():
        if dep_path not in sys.path:
            sys.path.append(dep_path)
