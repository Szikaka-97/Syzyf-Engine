# Shit that needs doing:

## Assimp version

Assimp version 6.0.4 doesn't require the silly _MSC_VER hack to work, but breaks importing meshes with materials

## Resources database

Currently, resources are supposed to be cached after loading. However, not only doesn't it work, it also sometimes mistakes assets as loaded when they aren't in the database, causing it to return a nullptr and crash. Ideally, all resource loading would be moved to the Scene to ensure memory safety

## Move IBL generation to compute shaders

I mean come on, blitting on a mesh in 2026? *So passé*

## Graphics API additions

- ### `class FramebufferScope (optional)`
Have a C# `IDisposable` style class that controls the framebuffer stuff is currently rendered to

- ### `void DrawMeshNow(const Framebuffer* framebuffer, const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation)`
Draw a mesh to the selected framebuffer immediately, useful for stuff like reflection probe generation, where we might only want the skybox to be rendered

## Graphics fixes

- ### Frustum culling only slightly works, needs fixing
- ### Directional light shadow cascades sometimes cause weird areas of shade to appear in the scene, needs fixing
- ### Environmental probe sampling occasionally returns NaN, needs investigating
