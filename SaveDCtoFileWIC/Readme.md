# SaveDCtoFileWIC

A Windows DLL for saving a device context (HDC) to an image file using Windows Imaging Component (WIC).  
Supports PNG, JPEG, GIF, BMP, and TIFF formats.

## Features

- Export HDC to image file (auto-detects format from file extension)
- Supports alpha channel (32bpp BGRA)
- GIF palette and format conversion

## Usage
// Example usage in C++ extern "C" int SaveDCToFileWIC(HDC hdc, LPCWSTR filename);
// Save the contents of an HDC to a PNG file SaveDCToFileWIC(hdc, L"output.png");


## Build

- Visual Studio 2019 or later
- Windows 10 SDK

## License

MIT License