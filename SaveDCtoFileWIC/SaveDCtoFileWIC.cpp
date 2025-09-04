

#include "pch.h"
#include <windows.h>
#include <wincodec.h>
#include <wchar.h>

#ifndef WICBitmapPaletteTypeFixedWeb
#define WICBitmapPaletteTypeFixedWeb WICBitmapPaletteTypeFixed
#endif

#ifndef WICBitmapPaletteTypeFixed
#define WICBitmapPaletteTypeFixed WICBitmapPaletteTypeFixedWebPalette
#endif

// Define format constants
static bool GetFormatClsid(int format, CLSID* pClsid) {
    const int FORMAT_PNG = 0;
    const int FORMAT_JPEG = 1;
    const int FORMAT_GIF = 2;
    const int FORMAT_BMP = 3;
    const int FORMAT_TIFF = 4;

    // Debug output to verify format value (remove after debugging)
    WCHAR debugMsg[256];
    wsprintfW(debugMsg, L"GetFormatClsid: format=%d\n", format);
    OutputDebugStringW(debugMsg);

    switch (format) {
    case FORMAT_PNG:
        *pClsid = GUID_ContainerFormatPng;
        return true;
    case FORMAT_JPEG:
        *pClsid = GUID_ContainerFormatJpeg;
        return true;
    case FORMAT_GIF:
        *pClsid = GUID_ContainerFormatGif;
        return true;
    case FORMAT_BMP:
        *pClsid = GUID_ContainerFormatBmp;
        return true;
    case FORMAT_TIFF:
        *pClsid = GUID_ContainerFormatTiff;
        return true;
    default:
        return false;
    }
}

// Helper function to get format from file extension
static int GetFormatFromExtension(LPCWSTR filename) {
    const int FORMAT_PNG = 0;
    const int FORMAT_JPEG = 1;
    const int FORMAT_GIF = 2;
    const int FORMAT_BMP = 3;
    const int FORMAT_TIFF = 4;

    // Find the last dot in the filename
    LPCWSTR ext = wcsrchr(filename, L'.');
    if (!ext || !ext[1]) {
        // No extension or empty extension, default to BMP
        return FORMAT_BMP;
    }

    // Move past the dot
    ext++;

    // Compare extension case-insensitively
    if (_wcsicmp(ext, L"png") == 0) {
        return FORMAT_PNG;
    }
    else if (_wcsicmp(ext, L"jpg") == 0 || _wcsicmp(ext, L"jpeg") == 0) {
        return FORMAT_JPEG;
    }
    else if (_wcsicmp(ext, L"gif") == 0) {
        return FORMAT_GIF;
    }
    else if (_wcsicmp(ext, L"bmp") == 0) {
        return FORMAT_BMP;
    }
    else if (_wcsicmp(ext, L"tif") == 0 || _wcsicmp(ext, L"tiff") == 0) {
        return FORMAT_TIFF;
    }
    else {
        // Unrecognized extension, default to BMP
        return FORMAT_BMP;
    }
}

extern "C" __declspec(dllexport)
int SaveDCToFileWIC(HDC hdc, LPCWSTR filename) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return -1; // SAVE_DC_COINIT_FAILED

    // Determine format from filename extension
    int format = GetFormatFromExtension(filename);

    // Validate format
    CLSID formatClsid;
    if (!GetFormatClsid(format, &formatClsid)) {
        CoUninitialize();
        return -5; // SAVE_DC_INVALID_FORMAT
    }

    IWICImagingFactory* pFactory = nullptr;
    IWICBitmap* pBitmap = nullptr;
    IWICStream* pStream = nullptr;
    IWICBitmapEncoder* pEncoder = nullptr;
    IWICBitmapFrameEncode* pFrame = nullptr;
    IPropertyBag2* pProps = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    IWICBitmapSource* pSource = nullptr;
    IWICPalette* pPalette = nullptr;
    HDC memDC = nullptr;
    HBITMAP hBitmap = nullptr;
    RECT rc;
    int width = 0;
    int height = 0;
    WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;

    // Get dimensions from HDC
    GetClipBox(hdc, &rc);
    width = rc.right - rc.left;
    height = rc.bottom - rc.top;

    // Create GDI resources
    memDC = CreateCompatibleDC(hdc);
    if (!memDC) {
        CoUninitialize();
        return -3; // SAVE_DC_DC_FAILED
    }
    hBitmap = CreateCompatibleBitmap(hdc, width, height);
    if (!hBitmap) {
        DeleteDC(memDC);
        CoUninitialize();
        return -4; // SAVE_DC_BITMAP_FAILED
    }
    SelectObject(memDC, hBitmap);
    BitBlt(memDC, 0, 0, width, height, hdc, rc.left, rc.top, SRCCOPY);

    // Create WIC factory
    hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr,
        CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr)) {
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        CoUninitialize();
        return -2; // SAVE_DC_FACTORY_FAILED
    }

    // Create bitmap from HBITMAP
    hr = pFactory->CreateBitmapFromHBITMAP(hBitmap, nullptr,
        WICBitmapUseAlpha, &pBitmap);
    if (FAILED(hr)) goto cleanup;

    // Initialize stream
    hr = pFactory->CreateStream(&pStream);
    if (FAILED(hr)) goto cleanup;
    hr = pStream->InitializeFromFilename(filename, GENERIC_WRITE);
    if (FAILED(hr)) goto cleanup;

    // Create encoder
    hr = pFactory->CreateEncoder(formatClsid, nullptr, &pEncoder);
    if (FAILED(hr)) goto cleanup;

    hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
    if (FAILED(hr)) goto cleanup;

    // Create frame
    hr = pEncoder->CreateNewFrame(&pFrame, &pProps);
    if (FAILED(hr)) goto cleanup;

    hr = pFrame->Initialize(pProps);
    if (FAILED(hr)) goto cleanup;

    hr = pFrame->SetSize(width, height);
    if (FAILED(hr)) goto cleanup;

    // Set pixel format
    hr = pFrame->SetPixelFormat(&formatGUID);
    if (FAILED(hr)) goto cleanup;

    // Convert to GIF-compatible format if needed
    pSource = pBitmap;
    if (format == 2) { // GIF format
        hr = pFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr)) goto cleanup;

        // Create a custom palette for GIF
        hr = pFactory->CreatePalette(&pPalette);
        if (SUCCEEDED(hr)) {
            hr = pPalette->InitializeFromBitmap(pBitmap, 256, FALSE);
            if (FAILED(hr)) {
                // Fallback to fixed web palette
                hr = pPalette->InitializePredefined(WICBitmapPaletteTypeFixedWeb, FALSE);
            }
        }
        if (FAILED(hr)) goto cleanup;

        // Initialize format converter with palette
        hr = pConverter->Initialize(
            pBitmap,
            GUID_WICPixelFormat8bppIndexed,
            WICBitmapDitherTypeNone,
            pPalette,
            0.0,
            WICBitmapPaletteTypeCustom
        );
        if (SUCCEEDED(hr)) {
            pSource = pConverter;
            formatGUID = GUID_WICPixelFormat8bppIndexed;
        }
        else {
            goto cleanup; // Return -8 for format conversion failure
        }
    }

    // Write the source to the frame
    hr = pFrame->WriteSource(pSource, nullptr);
    if (FAILED(hr)) goto cleanup;

    hr = pFrame->Commit();
    if (FAILED(hr)) goto cleanup;

    hr = pEncoder->Commit();
    if (FAILED(hr)) goto cleanup;

cleanup:
    // Release all resources in reverse order
    if (pPalette) pPalette->Release();
    if (pConverter) pConverter->Release();
    if (pFrame) pFrame->Release();
    if (pProps) pProps->Release();
    if (pEncoder) pEncoder->Release();
    if (pStream) pStream->Release();
    if (pBitmap) pBitmap->Release();
    if (hBitmap) DeleteObject(hBitmap);
    if (memDC) DeleteDC(memDC);
    if (pFactory) pFactory->Release();
    CoUninitialize();

    // Return specific error codes
    if (SUCCEEDED(hr)) return 0;
    if (hr == E_INVALIDARG) return -5; // Invalid format
    if (format == 2 && hr == WINCODEC_ERR_WRONGSTATE) return -8; // GIF format conversion failure
    return -7; // Generic failure
}