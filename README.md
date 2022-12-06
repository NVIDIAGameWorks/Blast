# Blast SDK Repo

Online documentation may be found here: [Blast SDK Documentation](http://omniverse-docs.s3-website-us-east-1.amazonaws.com/blast-sdk).

## Building the SDK

### Windows
1. build: run `build.bat`
2. built sdk location: `_build\windows-x86_64\release\blast-sdk` (release), `_build\windows-x86_64\debug\blast-sdk` (debug) 

### Linux
0. initialize (once): run `./setup.sh`
1. build: run `./build.sh`
2. built sdk location: `_build/linux-x86_64/release/blast-sdk` (release), `_build/linux-x86_64/debug/blast-sdk` (debug) 

## Building Documentation

### Windows
1. build: run `repo.bat docs`
2. built docs location: `_build\docs\blast-sdk\latest`

### Linux
1. build: run `./repo.sh docs`
2. built docs location: `_build/docs/blast-sdk/latest`
