import zipfile
import os
import sys
import subprocess

__author__ = 'hfannar'


def get_root_path():
    my_dir = os.path.dirname(os.path.realpath(__file__))
    path_prefix = os.path.abspath(os.path.join(my_dir, "../.."))
    return path_prefix


def is_signed(filepath):
    path_prefix = get_root_path()
    verify_cmd = os.path.join(path_prefix, 'buildtools/signtool/verify.cmd')
    # we want silent execution of this command so redirect output to dev null:
    black_hole = open(os.devnull, 'wb')
    status = subprocess.call([verify_cmd, filepath], stdout=black_hole, stderr=black_hole)
    return True if status == 0 else False


def sign(filepath):
    path_prefix = get_root_path()
    cmd = os.path.join(path_prefix, 'buildtools/signtool/sign.cmd')
    # we want silent execution of this command so redirect output to dev null:
    black_hole = open(os.devnull, 'wb')
    status = subprocess.call([cmd, filepath], stdout=black_hole, stderr=black_hole)
    return True if status == 0 else False


def create_package(package_path, iterable_contents):
    try:
        # Add extension if needed:
        zip_ext = '.zip'
        if not package_path.endswith(zip_ext):
            package_path += zip_ext
        if os.path.exists(package_path):
            os.remove(package_path) # next operation won't overwrite zip file if it's locked, it will fail here in that case
        with zipfile.ZipFile(package_path, 'w') as arc:
            for full_path, rel_path in iterable_contents:
                arc.write(full_path, rel_path)
                #print rel_path
                """
                for relative_path in relative_paths:
                    basename = os.path.basename(relative_path)
                    basename_no_ext = os.path.splitext(basename)[0]
                    path_in_zip = os.path.join(folder, basename)
                    path_in_filesystem = os.path.join(path_prefix, relative_path)

                    if basename.endswith('dll') or basename.endswith('exe'):
                        # sign executables that aren't signed:
                        if not is_signed(path_in_filesystem):
                            print 'Signing:', path_in_filesystem, '...'
                            ok = sign(path_in_filesystem)
                            if not ok:
                                print 'Failed to sign:', path_in_filesystem
                                raise WindowsError()
                """

    except WindowsError:
        # On failure we don't want to leave a half-baked zip file
        os.remove(package_path)
        raise

    print 'Package %s has been created.' % package_path
