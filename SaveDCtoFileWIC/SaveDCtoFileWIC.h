#pragma once
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

    // Supported image formats
    enum ImageFormat {
        FORMAT_PNG  = 0,  // Lossless, supports transparency
        FORMAT_JPEG = 1,  // Compressed, good for photos
        FORMAT_BMP  = 2,  // Uncompressed, large file size
        FORMAT_TIFF = 3,  // Multi-page support, used in scanning
        FORMAT_GIF  = 4   // Limited colors, supports animation (single-frame only here)
    };


    // Save an HDC to an image file using WIC
    __declspec(dllexport)
        int SaveDCToFileWIC(HDC hdc, LPCWSTR filename, int format);

#ifdef __cplusplus
}
#endif

