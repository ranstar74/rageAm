# rageAm for GTA V

How it works? See here https://www.youtube.com/watch?v=-uz1p3hUxSo

This is a library / toolkit that is focused on realtime development without need to reboot game every time.

Note that this is still in alpha/concept stage so it's not recommented to attempt to use it.

Currently supported features:
* Material Editor for Fragments & Drawables
  * Replace textures by dragging .DDS or any other texture format in folder
  * Edit any available parameter in real time
* Shader Editor
  * Replace game shaders by simply placing .HLSL sources in folder
* Light Editor
  * Supports all light types - Point, Spot, Capsule
  * Outlines and gizmos for lights in game viewport
  * Cull Plane Gizmo
  * All lights imported from 3D scene file with transform preserved
* Drawable Import
  * Skinning + linking nodes to bones
  * Embed dictionary
  * Geometry bounds (with correct shrinking & octants)
  * No vertex count limits, geometries will be split on smaller chunks
  * Vertex buffers are automatically composed for game shaders
  * In game viewer - free play, free cam & orbit cam
