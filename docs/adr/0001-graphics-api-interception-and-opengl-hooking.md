# ADR 0001: Graphics API Interception & OpenGL Hooking

## Status
Accepted

## Context
*The Chronicles of Riddick: Escape from Butcher Bay* (both the original 2004 PC release and the 2009 *Assault on Dark Athena* PC remaster containing the remastered campaign) is commonly assumed to be a DirectX 9 title due to its release era and installer dependencies. 

However, deep technical inspection of `DarkAthena.exe` confirms that Starbreeze's proprietary engine (`sbengine` / `XRengine`) runs natively on **OpenGL** on PC, utilizing WGL for context creation and custom ARB/GLSL shader pipelines for stencil shadow volumes and normal mapping.

In comparison, *F.E.A.R. VR* injected a custom `d3d9.dll` to intercept `IDirect3DDevice9`. If we were to assume Riddick was DirectX 9, a `d3d9.dll` hook would remain uninvoked or would intercept only auxiliary non-rendering calls.

## Decision
1. **Target API:** We will intercept the rendering pipeline via a custom 32-bit proxy **`opengl32.dll`** placed in the `System/Win32_x86/` game directory alongside `DarkAthena.exe`.
2. **Hooking Targets:**
   - `wglCreateContext`, `wglMakeCurrent`, and `wglSwapBuffers` to detect graphics device initialization, track frame lifecycles, and coordinate swapchain submission.
   - `wglGetProcAddress` to detour modern OpenGL functions, notably:
     - Framebuffer operations (`glBindFramebuffer`, `glGenFramebuffers`)
     - Viewport manipulations (`glViewport`, `glScissor`)
     - Shader program and uniform upload calls (`glUniformMatrix4fv`, `glProgramLocalParameter4fARB`)
3. **Texture Interoperability:**
   To bridge OpenGL textures to OpenXR runtimes that prefer DirectX 11 or Vulkan, we will utilize `WGL_NV_DX_interop` / `WGL_NV_DX_interop2` where available, sharing the rendered OpenGL FBO texture with a Direct3D 11 / DXGI texture handle without costly CPU-side readbacks.

## Consequences
### Positive
- Works natively with the Starbreeze Engine's genuine PC graphics pipeline without translation overhead.
- Leverages proven patterns from classic OpenGL VR ports (such as *Doom 3 VR: Fully Possessed* and *Quake VR*).
- Allows direct capture of intermediate render targets before post-processing and UI overlays.

### Negative / Challenges
- Modern graphics drivers (especially NVIDIA) require careful handling of OpenGL extension limits and present modes (e.g. extension limit profile settings or TCoRFix compatibility).
- OpenXR's `XR_KHR_opengl_enable` extension has varying vendor driver quality compared to `XR_KHR_D3D11_enable`, reinforcing the need for the `WGL_NV_DX_interop` bridge.
