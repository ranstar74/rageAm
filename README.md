# rageAm for GTA V

### Join our [discord](https://discord.gg/tn5NXB8zFd) to download releases, and get the most up-to-date development information

This library / toolkit is focused on realtime development without the need to reboot the game every time.

How it works? See here https://www.youtube.com/watch?v=-uz1p3hUxSo

# Building from source
Install [Autodesk FBX SDK](https://aps.autodesk.com/developer/overview/fbx-sdk) and add environment variable `FBXSDK` with path to it, example: `C:/Program Files/Autodesk/FBX SDK/2020.3.7`

Install gRPC: (*Takes ~18 minutes on Ryzen 3700X*)
```
vcpkg install grpc:x64-windows-static-md
```

# Currently supported features:
* Drawable Editor
  * Skinning + linking nodes to bones
  * Embed dictionary
  * Geometry bounds (with correct shrinking & octants)
  * No vertex count limits, geometries will be split on smaller chunks
  * Vertex buffers are automatically composed for game shaders
  * In game viewer - free play, free cam & orbit cam
  * Light Editor
    * Supports all light types - Point, Spot, Capsule
    * Outlines and gizmos for lights in game viewport
    * Cull Plane Gizmo
    * All lights imported from 3D scene file with transform preserved
  * Material Editor
    * Sync with the texture dictionary editor
    * Edit any available parameter in real time
* TXD Editor
  * Fast and high-quality BC encoders
  * Mip-map preview with linear/point samplers
  * Post-processing:
    * Cutout Alpha / Alpha Coverage
    * Max Size
    * Brightness / Contrast
  * Texture presets
* Model Inspector
  * Load YDR files and inspect various properties
