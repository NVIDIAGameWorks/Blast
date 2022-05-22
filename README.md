# Blast SDK Repo

Online documentation may be found here: [Blast SDK Documentation](http://omniverse-docs.s3-website-us-east-1.amazonaws.com/blast-sdk).

## Building the SDK

### Windows
1. build: `build.bat -c release`
2. run: `_build\windows-x86_64\release\omni.app.blast.bat`

### Linux
0. initialize (once): `./setup.sh`
1. build: `./build.sh -c release`
2. run: `_build/windows-x86_64/release/omni.app.blast.sh`

## Building Documentation

### Windows
1. run: `repo.bat docs`

### Linux
1. run: `./repo.sh docs`
