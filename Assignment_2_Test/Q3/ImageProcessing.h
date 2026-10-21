#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H

#include "Canvas.h"
#include <string>
#include <cstdint>
#include <fstream>              // used only by the BMP routines below

// Only the declarations are needed here. The test file defines
// STB_IMAGE_IMPLEMENTATION / STB_IMAGE_WRITE_IMPLEMENTATION itself and
// includes the stb headers BEFORE this file. The stb implementation part
// has no include guard, so including it a second time here (while the
// macro is still defined) would compile the whole library twice.
// The checks below include each stb header only if it is not already in.
#ifndef STBI_INCLUDE_STB_IMAGE_H
#include "stb_image.h"
#endif
#ifndef INCLUDE_STB_IMAGE_WRITE_H
#include "stb_image_write.h"
#endif

class ImageProcessing {
public:
    // 1. Ingestion & Transcoding to BMP:
    //    Loads a PNG or JPG file and writes it out as a 24-bit uncompressed BMP file.
    bool TranscodeToBMP(const std::string& inputPngJpgPath,
        const std::string& outputBmpPath);

    // 2. Basic Processing Pipeline (overloaded):
    //    Loads a BMP file, applies brightness scaling and/or color inversion, and saves as a PNG.
    bool ProcessSingleImage(const std::string& inputBmpPath,
        const std::string& outputPngPath,
        float brightnessFactor,
        bool invertColors);

    //    Same pipeline, but with independent red/green/blue brightness factors.
    bool ProcessSingleImage(const std::string& inputBmpPath,
        const std::string& outputPngPath,
        float rFactor, float gFactor, float bFactor,
        bool invertColors);

    // 3. Stitching:
    //    Loads two BMP files, combines them horizontally (if horizontal == true) or vertically
    //    (if horizontal == false), and saves the result to outputPngPath as a PNG.
    bool StitchImages(const std::string& inputBmpPathA,
        const std::string& inputBmpPathB,
        const std::string& outputPngPath,
        bool horizontal);

    // 4. Sub-canvas Operations:
    bool CropAndSave(const std::string& inputBmpPath,
        const std::string& outputPngPath,
        int x, int y, int w, int h);

    //    Overwrites pixels of the base image with the overlay.
    bool PasteImage(const std::string& baseBmpPath,
        const std::string& overlayBmpPath,
        const std::string& outputPngPath,
        int destX, int destY);

    //    Overload: overlay pixels equal to (keyR, keyG, keyB) are treated as transparent.
    bool PasteImage(const std::string& baseBmpPath,
        const std::string& overlayBmpPath,
        const std::string& outputPngPath,
        int destX, int destY,
        uint8_t keyR, uint8_t keyG, uint8_t keyB);

    // 5. Geometry:
    //    Loads a BMP, applies geometric transform (0 = FlipH, 1 = FlipV, 2 = Rotate90CW), and saves as a PNG.
    bool TransformGeometry(const std::string& inputBmpPath,
        const std::string& outputPngPath,
        int transformType);

    // 6. Convolution:
    //    Loads a BMP, applies a 2D spatial convolution with an arbitrary square kernel, and saves as a PNG.
    bool ApplyConvolution(const std::string& inputBmpPath,
        const std::string& outputPngPath,
        const float* kernelWeights,
        int kernelSize);

private:
    // ---- File helpers (all return true on success) ----
    bool loadPngJpg(const std::string& path, Canvas& out);
    bool savePng(const std::string& path, const Canvas& canvas);
    bool loadBmp(const std::string& path, Canvas& out);
    bool saveBmp(const std::string& path, const Canvas& canvas);

    // ---- Little-endian byte helpers for the BMP headers ----
    static void writeU16(uint8_t* dest, uint16_t value);
    static void writeU32(uint8_t* dest, uint32_t value);
    static uint16_t readU16(const uint8_t* src);
    static uint32_t readU32(const uint8_t* src);
    static int bmpRowBytes(int width);   // width * 3 rounded up to a multiple of 4
};

// ===================================================================
// Little-endian helpers (lowest byte first)
// ===================================================================

inline void ImageProcessing::writeU16(uint8_t* dest, uint16_t value) {
    dest[0] = (uint8_t)(value & 0xFF);
    dest[1] = (uint8_t)((value >> 8) & 0xFF);
}

inline void ImageProcessing::writeU32(uint8_t* dest, uint32_t value) {
    dest[0] = (uint8_t)(value & 0xFF);
    dest[1] = (uint8_t)((value >> 8) & 0xFF);
    dest[2] = (uint8_t)((value >> 16) & 0xFF);
    dest[3] = (uint8_t)((value >> 24) & 0xFF);
}

inline uint16_t ImageProcessing::readU16(const uint8_t* src) {
    return (uint16_t)(src[0] | (src[1] << 8));
}

inline uint32_t ImageProcessing::readU32(const uint8_t* src) {
    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
        ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

inline int ImageProcessing::bmpRowBytes(int width) {
    int rowBytes = width * 3;
    int padding = (4 - (rowBytes % 4)) % 4;
    return rowBytes + padding;
}

// ===================================================================
// PNG / JPG (stb)
// ===================================================================

inline bool ImageProcessing::loadPngJpg(const std::string& path, Canvas& out) {
    int w = 0;
    int h = 0;
    int channelsInFile = 0;
    unsigned char* bytes = stbi_load(path.c_str(), &w, &h, &channelsInFile, 3);
    if (bytes == nullptr) {
        return false;
    }
    out.loadFromRaw(bytes, w, h, w * 3);   // copies the bytes into the Canvas
    stbi_image_free(bytes);                // free the temporary stb buffer
    return out.getRawData() != nullptr;    // false if the image is empty
}

inline bool ImageProcessing::savePng(const std::string& path, const Canvas& canvas) {
    if (canvas.getRawData() == nullptr) {
        return false;                      // nothing to save
    }
    int w = canvas.getWidth();
    int h = canvas.getHeight();
    // Pixel is exactly 3 bytes (r, g, b), so the buffer is plain RGB bytes.
    int ok = stbi_write_png(path.c_str(), w, h, 3, canvas.getRawData(), w * 3);
    return ok != 0;
}

// ===================================================================
// Handwritten 24-bit BMP writer
// ===================================================================

inline bool ImageProcessing::saveBmp(const std::string& path, const Canvas& canvas) {
    if (canvas.getRawData() == nullptr) {
        return false;
    }
    int width = canvas.getWidth();
    int height = canvas.getHeight();
    int rowBytes = bmpRowBytes(width);
    uint32_t imageSize = (uint32_t)rowBytes * (uint32_t)height;

    // ---- Build the 54-byte header ----
    uint8_t header[54];
    for (int i = 0; i < 54; i++) {
        header[i] = 0;
    }
    // File header (14 bytes)
    header[0] = 'B';
    header[1] = 'M';
    writeU32(header + 2, 54 + imageSize);   // file size
    writeU16(header + 6, 0);                // reserved 1
    writeU16(header + 8, 0);                // reserved 2
    writeU32(header + 10, 54);              // pixel data offset
    // Info header (40 bytes)
    writeU32(header + 14, 40);              // info header size
    writeU32(header + 18, (uint32_t)width);
    writeU32(header + 22, (uint32_t)height); // positive = bottom-up rows
    writeU16(header + 26, 1);               // planes
    writeU16(header + 28, 24);              // bits per pixel
    writeU32(header + 30, 0);               // compression (none)
    writeU32(header + 34, imageSize);       // image size
    writeU32(header + 38, 2835);            // horizontal resolution
    writeU32(header + 42, 2835);            // vertical resolution
    writeU32(header + 46, 0);               // colours used
    writeU32(header + 50, 0);               // important colours

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(header), 54);

    // ---- Pixel rows: bottom row first, B G R order, zero padding ----
    uint8_t* rowBuffer = new uint8_t[rowBytes];
    for (int i = 0; i < rowBytes; i++) {
        rowBuffer[i] = 0;                   // padding bytes stay 0
    }
    for (int fileRow = 0; fileRow < height; fileRow++) {
        int y = height - 1 - fileRow;       // file row 0 = bottom of image
        for (int x = 0; x < width; x++) {
            const Pixel& p = canvas.at(x, y);
            rowBuffer[x * 3 + 0] = p.b;
            rowBuffer[x * 3 + 1] = p.g;
            rowBuffer[x * 3 + 2] = p.r;
        }
        file.write(reinterpret_cast<const char*>(rowBuffer), rowBytes);
    }
    delete[] rowBuffer;

    file.close();
    return !file.fail();
}

// ===================================================================
// Handwritten 24-bit BMP reader
// ===================================================================

inline bool ImageProcessing::loadBmp(const std::string& path, Canvas& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // Total file size (needed for the "enough bytes" check)
    file.seekg(0, std::ios::end);
    long long fileSize = (long long)file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize < 54) {
        return false;
    }

    uint8_t header[54];
    file.read(reinterpret_cast<char*>(header), 54);
    if (file.gcount() != 54) {
        return false;
    }

    // ---- Validate the header ----
    if (header[0] != 'B' || header[1] != 'M') {
        return false;
    }
    uint32_t dataOffset = readU32(header + 10);
    int32_t width = (int32_t)readU32(header + 18);
    int32_t height = (int32_t)readU32(header + 22);
    uint16_t bitsPerPixel = readU16(header + 28);
    uint32_t compression = readU32(header + 30);

    if (bitsPerPixel != 24 || compression != 0) {
        return false;
    }
    if (width <= 0 || height <= 0) {
        return false;
    }

    // ---- Make sure the file holds every pixel row ----
    long long rowBytes = (long long)width * 3;
    rowBytes = rowBytes + (4 - (rowBytes % 4)) % 4;
    long long bytesAfterOffset = fileSize - (long long)dataOffset;
    if (bytesAfterOffset <= 0) {
        return false;
    }
    // Same as rowBytes * height > bytesAfterOffset, but cannot overflow.
    if (height > bytesAfterOffset / rowBytes) {
        return false;
    }

    // ---- Read pixel rows into a temporary canvas ----
    Canvas loaded(width, height);
    uint8_t* rowBuffer = new uint8_t[rowBytes];
    file.seekg((long long)dataOffset, std::ios::beg);

    bool ok = true;
    for (int fileRow = 0; fileRow < height; fileRow++) {
        file.read(reinterpret_cast<char*>(rowBuffer), rowBytes);
        if (file.gcount() != rowBytes) {
            ok = false;
            break;
        }
        int y = height - 1 - fileRow;       // file row 0 = bottom of image
        for (int x = 0; x < width; x++) {
            Pixel& p = loaded.at(x, y);
            p.b = rowBuffer[x * 3 + 0];
            p.g = rowBuffer[x * 3 + 1];
            p.r = rowBuffer[x * 3 + 2];
        }                                   // padding bytes are skipped
    }
    delete[] rowBuffer;

    if (!ok) {
        return false;
    }
    out.copyFrom(loaded);
    return true;
}

// ===================================================================
// Public pipeline functions
// ===================================================================

inline bool ImageProcessing::TranscodeToBMP(const std::string& inputPngJpgPath,
    const std::string& outputBmpPath) {
    Canvas image(0, 0);
    if (!loadPngJpg(inputPngJpgPath, image)) {
        return false;
    }
    return saveBmp(outputBmpPath, image);
}

inline bool ImageProcessing::ProcessSingleImage(const std::string& inputBmpPath,
    const std::string& outputPngPath,
    float brightnessFactor,
    bool invertColors) {
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)) {
        return false;
    }
    Canvas result = image.scaleBrightness(brightnessFactor);   // brightness first
    if (invertColors) {
        Canvas inverted = result.invertColors();               // then inversion
        return savePng(outputPngPath, inverted);
    }
    return savePng(outputPngPath, result);
}

inline bool ImageProcessing::ProcessSingleImage(const std::string& inputBmpPath,
    const std::string& outputPngPath,
    float rFactor, float gFactor, float bFactor,
    bool invertColors) {
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)) {
        return false;
    }
    Canvas result = image.scaleBrightness(rFactor, gFactor, bFactor);
    if (invertColors) {
        Canvas inverted = result.invertColors();
        return savePng(outputPngPath, inverted);
    }
    return savePng(outputPngPath, result);
}

inline bool ImageProcessing::StitchImages(const std::string& inputBmpPathA,
    const std::string& inputBmpPathB,
    const std::string& outputPngPath,
    bool horizontal) {
    Canvas imageA(0, 0);
    Canvas imageB(0, 0);
    if (!loadBmp(inputBmpPathA, imageA) || !loadBmp(inputBmpPathB, imageB)) {
        return false;
    }
    if (horizontal) {
        Canvas result = imageA.stitchHorizontal(imageB);
        return savePng(outputPngPath, result);
    }
    Canvas result = imageA.stackVertical(imageB);
    return savePng(outputPngPath, result);
}

inline bool ImageProcessing::CropAndSave(const std::string& inputBmpPath,
    const std::string& outputPngPath,
    int x, int y, int w, int h) {
    if (w <= 0 || h <= 0 || x < 0 || y < 0) {
        return false;
    }
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)) {
        return false;
    }
    // The whole rectangle must be inside the image.
    // (Written as x > width - w instead of x + w > width to avoid overflow.)
    if (x > image.getWidth() - w || y > image.getHeight() - h) {
        return false;
    }
    Canvas result = image.crop(x, y, w, h);
    return savePng(outputPngPath, result);
}

inline bool ImageProcessing::PasteImage(const std::string& baseBmpPath,
    const std::string& overlayBmpPath,
    const std::string& outputPngPath,
    int destX, int destY) {
    Canvas base(0, 0);
    Canvas overlay(0, 0);
    if (!loadBmp(baseBmpPath, base) || !loadBmp(overlayBmpPath, overlay)) {
        return false;
    }
    base.paste(overlay, destX, destY);
    return savePng(outputPngPath, base);
}

inline bool ImageProcessing::PasteImage(const std::string& baseBmpPath,
    const std::string& overlayBmpPath,
    const std::string& outputPngPath,
    int destX, int destY,
    uint8_t keyR, uint8_t keyG, uint8_t keyB) {
    Canvas base(0, 0);
    Canvas overlay(0, 0);
    if (!loadBmp(baseBmpPath, base) || !loadBmp(overlayBmpPath, overlay)) {
        return false;
    }
    Pixel key;
    key.r = keyR;
    key.g = keyG;
    key.b = keyB;
    base.paste(overlay, destX, destY, key);
    return savePng(outputPngPath, base);
}

inline bool ImageProcessing::TransformGeometry(const std::string& inputBmpPath,
    const std::string& outputPngPath,
    int transformType) {
    if (transformType < 0 || transformType > 2) {
        return false;
    }
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)) {
        return false;
    }
    if (transformType == 0) {
        Canvas result = image.flipHorizontal();
        return savePng(outputPngPath, result);
    }
    if (transformType == 1) {
        Canvas result = image.flipVertical();
        return savePng(outputPngPath, result);
    }
    Canvas result = image.rotate90Clockwise();
    return savePng(outputPngPath, result);
}

inline bool ImageProcessing::ApplyConvolution(const std::string& inputBmpPath,
    const std::string& outputPngPath,
    const float* kernelWeights,
    int kernelSize) {
    if (kernelWeights == nullptr || kernelSize < 1 || kernelSize % 2 == 0) {
        return false;
    }
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)) {
        return false;
    }
    // We only read the caller's weights; we never free or keep the pointer.
    Canvas result = image.applyFilter(kernelWeights, kernelSize);
    return savePng(outputPngPath, result);
}

#endif // IMAGE_PROCESSING_H

