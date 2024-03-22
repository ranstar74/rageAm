# rageAm for GTA V

### Join us [discord](https://discord.gg/tn5NXB8zFd) to get releases and most actual development information

This is a library / toolkit that is focused on realtime development without need to reboot game every time.

How it works? See here https://www.youtube.com/watch?v=-uz1p3hUxSo

Currently supported features:
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
