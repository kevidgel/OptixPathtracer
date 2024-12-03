# OptixPathtracer

Very simple implementation of a pathtracer with OpenGL + OptiX 7.7 (via OWL).

![OptixPathtracerImage](/OptixPathtracer.png)

## Motivation
Currently, the only graphics APIs with hardware raytracing acceleration are Vulkan, DX12, and NVIDIA's OptiX, and the former two are cumbersome to setup and develop with.
Of course, they offer much finer grained control and potentially better performance.
However, using OpenGL + OptiX allows us to achieve near-realtime levels of performance with much less code, allowing for more time spent developing core logic.

## Completed

- [x] GLFW + GLAD for rendering base
- [x] OptiX + OpenGL interop using pixel buffer unpacking
- [x] Basic raytracing in one weekend impl with rotating camera
- [x] User input to move camera + sample accumulation when camera is still
- [x] Performance improvements with: russian roulette
- [x] Environment mapping
- [x] Loading models

## TODO
- [ ] Loading models with textures
- [ ] Importance sampling environment maps
- [ ] Textures (as opposed to hardcoded colors per prim)
- [ ] Specular materials
- [ ] Importance sampling, better materials, MIS, ...

In the distant future:

- [ ] ReSTIR

## Installation + Build
I've only built this on Debian 12. I have no idea if it will build on Windows. It will not build on MacOS.

To install:

```bash
git clone <this repo> --recurse-submodules
```

To build (You can also use CLion):

```bash
cd $PROJECT_ROOT # This is your installation path
mkdir build
cd build
# Make sure to set your VCPKG_ROOT and OptiX_ROOT_DIR!
cmake .. -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake -DOptiX_ROOT_DIR=$OptiX_ROOT_DIR # -G Ninja if you want
make # or ninja
```

To run:
```bash
cd $PROJECT_BUILD_PATH/src # This is where the executable is stored
./renderer 
--model-path <path to obj>
--env-map <path to hdr>

# NOTE: On devices with NVIDIA Optimus (two devices), OpenGL might use the non-NVIDIA gpu. To fix (at least on Linux)
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia ./renderer ...
```
## Controls
Use WASD to move around the xz plane.
Use SPACE and LSHIFT to move up and down.
Use arrow keys to look around.

## Dependencies
System with OpenGL 4.6 capability. I use `glfw` + `glad`.

Make sure to have CUDA Toolkit version 12.6 installed.
You might run into issues with CMake, and may need to specify `-DCMAKE_CUDA_ARCHITECTURES` and `-DCMAKE_CUDA_COMPILER`.

Make sure to have OptiX 7.7 installed. Provide installation path as `-DOptiX_ROOT_DIR` if needed.
You NEED an NVIDIA GPU that supports this version of OptiX. You can install it here: https://developer.nvidia.com/designworks/optix/downloads/legacy.

### vcpkg
I use (prefer) vcpkg for most of my dependencies. Provide vcpkg toolchain file as `-DCMAKE_TOOLCHAIN_FILE` to cmake if needed. They are located in `/vcpkg.json`

### git submodules
I use:

- `owl` for Optix 7.7 wrapper API.
- `rapidobj` for obj loading.
- `argparse` for command line argument parsing.

You can install these via `git submodule`.

## Sources
I used a lot of help.
- https://learnopengl.com/
- https://raytracing.github.io/
- https://github.com/owl-project/owl/tree/master/samples
- https://github.com/ingowald/optix7course


