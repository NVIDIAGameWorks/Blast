# Blast SDK Repo

Online documentation may be found here: [Blast SDK Documentation](https://nvidia-omniverse.github.io/PhysX/blast/index.html).

## Building with repo tools

The default build contains the Blast SDK and unit tests. PhysX-dependent
extensions are optional and are generated only when a PhysX SDK path is
provided.

### Prerequisites

- Windows: Visual Studio 2019 with the Desktop development with C++ workload
  and a Windows 10 or 11 SDK. The build links the locally installed toolchain;
  MSVC and the Windows SDK are not downloaded as packages.
- Linux: a native C++ build toolchain available on the host.
- PhysX extensions and samples: an unpacked PhysX SDK binary distribution.
  Its root must contain `include/PxPhysicsAPI.h`, and its binary directory must
  contain `debug` and/or `release` subdirectories.

Dependencies managed by Packman are downloaded from the public CloudFront
package remote.

### SDK and unit tests

On Windows:

```bat
repo.bat build -r
```

On Linux:

```sh
./repo.sh build -r
```

Release output is written to
`_build/<platform>/release/blast-sdk`; use `-d` instead of `-r` for a
debug-only build.

### PhysX extensions

Pass the root of the unpacked PhysX distribution:

```bat
repo.bat build -r --physx-path C:\path\to\physx
```

When the PhysX root contains more than one binary-platform directory, select
one explicitly:

```bat
repo.bat build -r --physx-path C:\path\to\physx --physx-lib-path C:\path\to\physx\bin\win.x86_64.vc141.md
```

Legacy PhysX distributions may keep their foundation headers in a separate
PxShared package. Pass the root of that package as well:

```bat
repo.bat build -r --physx-path C:\path\to\physx --pxshared-path C:\path\to\pxshared
```

`--pxshared-path` must contain `include\foundation\PxTransform.h`. It is not
needed when that header is already present below the PhysX root.

Without `--physx-path`, `NvBlastExtPhysX` and
`NvBlastExtPxSerialization` are intentionally omitted.

### SampleAssetViewer (Windows)

The sample is opt-in and requires PhysX:

```bat
repo.bat build -r --samples --physx-path C:\path\to\physx
```

The executable, required DLLs, configuration, and shaders are staged in
`_build\windows-x86_64\release\blast-sdk\bin`. Run
`SampleAssetViewer.exe` from that directory.
