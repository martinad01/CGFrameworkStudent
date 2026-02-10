#ifndef GLCOMPAT_H
#define GLCOMPAT_H

/*
 Compatibility layer for building with or without AGL on macOS.
 - On older SDKs/machines where AGL is present, define USES_AGL and include <AGL/agl.h>.
 - On modern SDKs where AGL is removed, include CGL and OpenGL headers instead.

 You can force a path via build settings by defining FORCE_USE_AGL=1 or FORCE_NO_AGL=1.
*/

#if defined(__APPLE__)
  #include <TargetConditionals.h>
  #include <AvailabilityMacros.h>
#endif

/* Allow build-system overrides */
#if defined(FORCE_USE_AGL) && FORCE_USE_AGL
  #define USES_AGL 1
#elif defined(FORCE_NO_AGL) && FORCE_NO_AGL
  #define USES_AGL 0
#else
  /* Heuristic: if the SDK provides AGL headers, prefer them; otherwise fallback. */
  /* We can't #include conditionally based on existence portably, so gate by macOS version macros. */
  /* AGL was removed from recent SDKs; assume no AGL for macOS 11+ SDKs. */
  #if defined(__APPLE__)
    #if defined(MAC_OS_X_VERSION_MAX_ALLOWED) && (MAC_OS_X_VERSION_MAX_ALLOWED < 110000)
      #define USES_AGL 1
    #else
      #define USES_AGL 0
    #endif
  #else
    #define USES_AGL 0
  #endif
#endif

#if USES_AGL
  /* Legacy AGL path */
  #include <AGL/agl.h>
  #include <OpenGL/gl.h>
#else
  /* Modern macOS path: use CGL for context management and OpenGL headers for rendering */
  #if defined(__APPLE__)
    #include <OpenGL/CGLCurrent.h>
    #include <OpenGL/CGLTypes.h>
    /* Prefer core profile if your project uses GL 3+ */
    #ifdef USE_GL3_CORE
      #include <OpenGL/gl3.h>
    #else
      #include <OpenGL/gl.h>
    #endif
  #endif
#endif

#endif /* GLCOMPAT_H */
