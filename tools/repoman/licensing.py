import os
import sys
import logging
import argparse

logger = logging.getLogger(os.path.basename(__file__))


def main():
    import repoman

    repoman.bootstrap()
    import omni.repo.man
    import omni.repo.licensing

    omni.repo.licensing.main()


if __name__ == "__main__":
    main()
