#ifndef IMAGE_PROCESSING_H
#define IMAGE_PROCESSING_H
#include "Canvas.h"
#include <string>
#include <cstdint>
#include <fstream>            
#ifndef STBI_INCLUDE_STB_IMAGE_H
#include "stb_image.h"
#endif
#ifndef INCLUDE_STB_IMAGE_WRITE_H
#include "stb_image_write.h"
#endif
class ImageProcessing{
public:
    bool TranscodeToBMP(const std::string& inputPngJpgPath,const std::string& outputBmpPath);
    bool ProcessSingleImage(const std::string& inputBmpPath,const std::string& outputPngPath,float brightnessFactor,bool invertColors);
    bool ProcessSingleImage(const std::string& inputBmpPath,const std::string& outputPngPath,float rFactor, float gFactor, float bFactor,bool invertColors);
    bool StitchImages(const std::string& inputBmpPathA,const std::string& inputBmpPathB,const std::string& outputPngPath,bool horizontal);
    bool CropAndSave(const std::string& inputBmpPath,const std::string& outputPngPath,int x, int y, int w, int h);
    bool PasteImage(const std::string& baseBmpPath,const std::string& overlayBmpPath,const std::string& outputPngPath,int destX, int destY);
    bool PasteImage(const std::string& baseBmpPath, const std::string& overlayBmpPath, const std::string& outputPngPath, int destX, int destY, uint8_t keyR, uint8_t keyG, uint8_t keyB);
    bool TransformGeometry(const std::string& inputBmpPath,const std::string& outputPngPath,int transformType);
    bool ApplyConvolution(const std::string& inputBmpPath,const std::string& outputPngPath,const float* kernelWeights,int kernelSize);
private:
    bool loadPngJpg(const std::string& path, Canvas& out);
    bool savePng(const std::string& path, const Canvas& canvas);
    bool loadBmp(const std::string& path, Canvas& out);
    bool saveBmp(const std::string& path, const Canvas& canvas);
    static void writeU16(uint8_t* dest, uint16_t value);
    static void writeU32(uint8_t* dest, uint32_t value);
    static uint16_t readU16(const uint8_t* src);
    static uint32_t readU32(const uint8_t* src);
    static int bmpRowBytes(int width);   
};
inline void ImageProcessing::writeU16(uint8_t* dest, uint16_t value){
    dest[0] = (uint8_t)(value & 0xFF);
    dest[1] = (uint8_t)((value >> 8) & 0xFF);
}
inline void ImageProcessing::writeU32(uint8_t* dest, uint32_t value){
    dest[0] = (uint8_t)(value & 0xFF);
    dest[1] = (uint8_t)((value >> 8) & 0xFF);
    dest[2] = (uint8_t)((value >> 16) & 0xFF);
    dest[3] = (uint8_t)((value >> 24) & 0xFF);
}
inline uint16_t ImageProcessing::readU16(const uint8_t* src){
    return (uint16_t)(src[0] | (src[1] << 8));
}
inline uint32_t ImageProcessing::readU32(const uint8_t* src){
    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
        ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}
inline int ImageProcessing::bmpRowBytes(int width) {
    int rowBytes = width * 3;
    int padding = (4 - (rowBytes % 4)) % 4;
    return rowBytes + padding;
}
inline bool ImageProcessing::loadPngJpg(const std::string& path, Canvas& out){
    int w = 0;
    int h = 0;
    int channelsInFile = 0;
    unsigned char* bytes = stbi_load(path.c_str(), &w, &h, &channelsInFile, 3);
    if (bytes == nullptr){
        return false;
    }
    out.loadFromRaw(bytes, w, h, w * 3);  
    stbi_image_free(bytes);              
    return out.getRawData() != nullptr;   
}
inline bool ImageProcessing::savePng(const std::string& path, const Canvas& canvas){
    if (canvas.getRawData() == nullptr){
        return false;                     
    }
    int w = canvas.getWidth();
    int h = canvas.getHeight();
    int ok = stbi_write_png(path.c_str(), w, h, 3, canvas.getRawData(), w * 3);
    return ok != 0;
}
inline bool ImageProcessing::saveBmp(const std::string& path, const Canvas& canvas){
    if (canvas.getRawData() == nullptr){
        return false;
    }
    int width = canvas.getWidth();
    int height = canvas.getHeight();
    int rowBytes = bmpRowBytes(width);
    uint32_t imageSize = (uint32_t)rowBytes * (uint32_t)height;
    uint8_t header[54];
    for (int i = 0; i < 54; i++){
        header[i] = 0;
    }
    header[0] = 'B';
    header[1] = 'M';
    writeU32(header + 2, 54 + imageSize);   
    writeU16(header + 6, 0);               
    writeU16(header + 8, 0);               
    writeU32(header + 10, 54);              
    writeU32(header + 14, 40);            
    writeU32(header + 18, (uint32_t)width);
    writeU32(header + 22, (uint32_t)height);
    writeU16(header + 26, 1);               
    writeU16(header + 28, 24);             
    writeU32(header + 30, 0);              
    writeU32(header + 34, imageSize);      
    writeU32(header + 38, 2835);            
    writeU32(header + 42, 2835);           
    writeU32(header + 46, 0);               
    writeU32(header + 50, 0);               
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()){
        return false;
    }
    file.write(reinterpret_cast<const char*>(header), 54);
    uint8_t* rowBuffer = new uint8_t[rowBytes];
    for (int i = 0; i < rowBytes; i++){
        rowBuffer[i] = 0;                   
    }
    for (int fileRow = 0; fileRow < height; fileRow++){
        int y = height - 1 - fileRow;       
        for (int x = 0; x < width; x++){
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
inline bool ImageProcessing::loadBmp(const std::string& path, Canvas& out){
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()){
        return false;
    }
    file.seekg(0, std::ios::end);
    long long fileSize = (long long)file.tellg();
    file.seekg(0, std::ios::beg);
    if (fileSize < 54){
        return false;
    }
    uint8_t header[54];
    file.read(reinterpret_cast<char*>(header), 54);
    if (file.gcount() != 54){
        return false;
    }
    if (header[0] != 'B' || header[1] != 'M'){
        return false;
    }
    uint32_t dataOffset = readU32(header + 10);
    int32_t width = (int32_t)readU32(header + 18);
    int32_t height = (int32_t)readU32(header + 22);
    uint16_t bitsPerPixel = readU16(header + 28);
    uint32_t compression = readU32(header + 30);
    if (bitsPerPixel != 24 || compression != 0){
        return false;
    }
    if (width <= 0 || height <= 0){
        return false;
    }
    long long rowBytes = (long long)width * 3;
    rowBytes = rowBytes + (4 - (rowBytes % 4)) % 4;
    long long bytesAfterOffset = fileSize - (long long)dataOffset;
    if (bytesAfterOffset <= 0){
        return false;
    }
    if (height > bytesAfterOffset / rowBytes){
        return false;
    }
    Canvas loaded(width, height);
    uint8_t* rowBuffer = new uint8_t[rowBytes];
    file.seekg((long long)dataOffset, std::ios::beg);
    bool ok = true;
    for (int fileRow = 0; fileRow < height; fileRow++){
        file.read(reinterpret_cast<char*>(rowBuffer), rowBytes);
        if (file.gcount() != rowBytes){
            ok = false;
            break;
        }
        int y = height - 1 - fileRow;      
        for (int x = 0; x < width; x++){
            Pixel& p = loaded.at(x, y);
            p.b = rowBuffer[x * 3 + 0];
            p.g = rowBuffer[x * 3 + 1];
            p.r = rowBuffer[x * 3 + 2];
        }                                  
    }
    delete[] rowBuffer;
    if (!ok){
        return false;
    }
    out.copyFrom(loaded);
    return true;
}
inline bool ImageProcessing::TranscodeToBMP(const std::string& inputPngJpgPath,const std::string& outputBmpPath){
    Canvas image(0, 0);
    if (!loadPngJpg(inputPngJpgPath, image)){
        return false;
    }
    return saveBmp(outputBmpPath, image);
}
inline bool ImageProcessing::ProcessSingleImage(const std::string& inputBmpPath,const std::string& outputPngPath,float brightnessFactor,bool invertColors){
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)){
        return false;
    }
    Canvas result = image.scaleBrightness(brightnessFactor);   
    if (invertColors){
        Canvas inverted = result.invertColors();               
        return savePng(outputPngPath, inverted);
    }
    return savePng(outputPngPath, result);
}
inline bool ImageProcessing::ProcessSingleImage(const std::string& inputBmpPath,const std::string& outputPngPath,float rFactor, float gFactor, float bFactor,bool invertColors){
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)){
        return false;
    }
    Canvas result = image.scaleBrightness(rFactor, gFactor, bFactor);
    if (invertColors){
        Canvas inverted = result.invertColors();
        return savePng(outputPngPath, inverted);
    }
    return savePng(outputPngPath, result);
}
inline bool ImageProcessing::StitchImages(const std::string& inputBmpPathA,const std::string& inputBmpPathB,const std::string& outputPngPath,bool horizontal){
    Canvas imageA(0, 0);
    Canvas imageB(0, 0);
    if (!loadBmp(inputBmpPathA, imageA) || !loadBmp(inputBmpPathB, imageB)){
        return false;
    }
    if (horizontal){
        Canvas result = imageA.stitchHorizontal(imageB);
        return savePng(outputPngPath, result);
    }
    Canvas result = imageA.stackVertical(imageB);
    return savePng(outputPngPath, result);
}
inline bool ImageProcessing::CropAndSave(const std::string& inputBmpPath,const std::string& outputPngPath,int x, int y, int w, int h){
    if (w <= 0 || h <= 0 || x < 0 || y < 0){
        return false;
    }
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)){
        return false;
    }
    if (x > image.getWidth() - w || y > image.getHeight() - h){
        return false;
    }
    Canvas result = image.crop(x, y, w, h);
    return savePng(outputPngPath, result);
}
inline bool ImageProcessing::PasteImage(const std::string& baseBmpPath,
    const std::string& overlayBmpPath,
    const std::string& outputPngPath,
    int destX, int destY){
    Canvas base(0, 0);
    Canvas overlay(0, 0);
    if (!loadBmp(baseBmpPath, base) || !loadBmp(overlayBmpPath, overlay)){
        return false;
    }
    base.paste(overlay, destX, destY);
    return savePng(outputPngPath, base);
}
inline bool ImageProcessing::PasteImage(const std::string& baseBmpPath,const std::string& overlayBmpPath,const std::string& outputPngPath,int destX, int destY,uint8_t keyR, uint8_t keyG, uint8_t keyB){
    Canvas base(0, 0);
    Canvas overlay(0, 0);
    if (!loadBmp(baseBmpPath, base) || !loadBmp(overlayBmpPath, overlay)){
        return false;
    }
    Pixel key;
    key.r = keyR;
    key.g = keyG;
    key.b = keyB;
    base.paste(overlay, destX, destY, key);
    return savePng(outputPngPath, base);
}
inline bool ImageProcessing::TransformGeometry(const std::string& inputBmpPath,const std::string& outputPngPath,int transformType){
    if (transformType < 0 || transformType > 2){
        return false;
    }
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)){
        return false;
    }
    if (transformType == 0){
        Canvas result = image.flipHorizontal();
        return savePng(outputPngPath, result);
    }
    if (transformType == 1){
        Canvas result = image.flipVertical();
        return savePng(outputPngPath, result);
    }
    Canvas result = image.rotate90Clockwise();
    return savePng(outputPngPath, result);
}
inline bool ImageProcessing::ApplyConvolution(const std::string& inputBmpPath,const std::string& outputPngPath,const float* kernelWeights,int kernelSize){
    if (kernelWeights == nullptr || kernelSize < 1 || kernelSize % 2 == 0){
        return false;
    }
    Canvas image(0, 0);
    if (!loadBmp(inputBmpPath, image)){
        return false;
    }
    Canvas result = image.applyFilter(kernelWeights, kernelSize);
    return savePng(outputPngPath, result);
}
#endif 

