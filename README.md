# OptixPathtracer

Very simple implementation of a pathtracer with OpenGL + OptiX 7.7 (via OWL).

## Motivation
Currently, the only graphics APIs with hardware raytracing acceleration are Vulkan and DX12, which are cumbersome to setup and develop with.
Of course, they offer much finer grained control and potentially better performance.
However, using OpenGL + OptiX allows us to achieve near-realtime levels of performance with much less code, allowing for more time spent developing core logic.

## Installation + Build
I've only built this on Debian 12. I have no idea if it will build on Windows. It will not build on Macs.

To install:

```bash
git clone <this repo> --recurse-submodules
```

To build (You can also use CLion):

```bash
cd $PROJECT_ROOT # This is your installation path
mkdir build
cd build
# Make sure to set your VCPKG_ROOT!
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake # -GNinja if you want
make # or ninja
```

To run:
```bash
./renderer

# NOTE: On devices with NVIDIA Optimus (two devices), OpenGL might use the non-NVIDIA gpu. To fix (at least on Linux)
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./renderer
```

## Dependencies
I use vcpkg for most of my dependencies. They are located in `/vcpkg.json`

Make sure to have CUDA Toolkit version 12.6 installed.

Make sure to have OptiX 7.7 installed. You NEED an NVIDIA GPU that supports this version of OptiX. You can install it here: https://developer.nvidia.com/designworks/optix/downloads/legacy.

The only other dependency is OWL, which is an OptiX API wrapper. You can install it via `git submodule`.
