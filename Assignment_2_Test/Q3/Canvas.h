#ifndef CANVAS_H
#define CANVAS_H
#include "Pixel.h"
#include "Kernel.h"
#include <cstdint>

class Canvas{
private:
    int width;
    int height;
    Pixel* data;
    Canvas& operator=(const Canvas&);
    void allocateBlack(int w, int h);
    static uint8_t scaleChannel(uint8_t value, float factor);
    static int clampInt(int value, int low, int high);
public:
    Canvas(int w, int h);
    ~Canvas();
    Canvas(const Canvas& other);
    void copyFrom(const Canvas& other);
    Pixel& at(int x, int y);
    const Pixel& at(int x, int y) const;
    void loadFromRaw(const void* sourceBytes, int srcWidth, int srcHeight,int sourceRowStrideBytes);
    Canvas scaleBrightness(float factor) const;
    Canvas scaleBrightness(float rFactor, float gFactor, float bFactor) const;
    Canvas invertColors() const;
    Canvas stitchHorizontal(const Canvas& rightSide) const;
    Canvas stackVertical(const Canvas& bottomSide) const;
    Canvas crop(int x, int y, int cropWidth, int cropHeight) const;
    void paste(const Canvas& source, int destX, int destY);
    void paste(const Canvas& source, int destX, int destY,const Pixel& transparentKey);
    Canvas flipHorizontal() const;
    Canvas flipVertical() const;
    Canvas rotate90Clockwise() const;
    Canvas applyFilter(const Kernel& kernel) const;
    Canvas applyFilter(const float* weights, int kernelSize) const;
    int getWidth() const;
    int getHeight() const;
    const Pixel* getRawData() const;
};

inline void Canvas::allocateBlack(int w, int h){
    if (w <= 0 || h <= 0){
        width = 0;
        height = 0;
        data = nullptr;
        return;
    }
    width = w;
    height = h;
    data = new Pixel[width * height];
    for (int i = 0; i < width * height; i++){
        data[i].r = 0;
        data[i].g = 0;
        data[i].b = 0;
    }
}
inline uint8_t Canvas::scaleChannel(uint8_t value, float factor){
    if (!(factor >= 0.0f)){      
        factor = 0.0f;
    }
    float result = value * factor + 0.5f;
    if (result > 255.0f){
        result = 255.0f;
    }
    if (!(result >= 0.0f)){
        result = 0.0f;
    }
    return (uint8_t)(int)result;
}
inline int Canvas::clampInt(int value, int low, int high){
    if (value < low) return low;
    if (value > high) return high;
    return value;
}
inline Canvas::Canvas(int w, int h){
    allocateBlack(w, h);
}
inline Canvas::~Canvas(){
    delete[] data;  
}
inline Canvas::Canvas(const Canvas& other){
    allocateBlack(other.width, other.height);
    for (int i = 0; i < width * height; i++){
        data[i] = other.data[i];
    }
}
inline void Canvas::copyFrom(const Canvas& other){
    if (this == &other){
        return;            
    }
    Pixel* newData = nullptr;
    int count = other.width * other.height;
    if (count > 0){
        newData = new Pixel[count];
        for (int i = 0; i < count; i++){
            newData[i] = other.data[i];
        }
    }
    delete[] data;
    data = newData;
    width = other.width;
    height = other.height;
}
inline Pixel& Canvas::at(int x, int y){
    return data[y * width + x];
}
inline const Pixel& Canvas::at(int x, int y) const{
    return data[y * width + x];
}
inline void Canvas::loadFromRaw(const void* sourceBytes, int srcWidth, int srcHeight,int sourceRowStrideBytes){
    if (sourceBytes == nullptr || srcWidth <= 0 || srcHeight <= 0){
        delete[] data;
        width = 0;
        height = 0;
        data = nullptr;
        return;
    }
    const uint8_t* bytes = static_cast<const uint8_t*>(sourceBytes);
    Pixel* newData = new Pixel[srcWidth * srcHeight];
    for (int y = 0; y < srcHeight; y++) {
        const uint8_t* row = bytes + (long long)y * sourceRowStrideBytes;
        for (int x = 0; x < srcWidth; x++) {
            Pixel& p = newData[y * srcWidth + x];
            p.r = row[x * 3 + 0];
            p.g = row[x * 3 + 1];
            p.b = row[x * 3 + 2];
        }
    }
    delete[] data;
    data = newData;
    width = srcWidth;
    height = srcHeight;
}
inline Canvas Canvas::scaleBrightness(float factor) const{
    return scaleBrightness(factor, factor, factor);
}
inline Canvas Canvas::scaleBrightness(float rFactor, float gFactor, float bFactor) const{
    Canvas result(width, height);
    for (int i = 0; i < width * height; i++){
        result.data[i].r = scaleChannel(data[i].r, rFactor);
        result.data[i].g = scaleChannel(data[i].g, gFactor);
        result.data[i].b = scaleChannel(data[i].b, bFactor);
    }
    return result;
}
inline Canvas Canvas::invertColors() const{
    Canvas result(width, height);
    for (int i = 0; i < width * height; i++){
        result.data[i].r = 255 - data[i].r;
        result.data[i].g = 255 - data[i].g;
        result.data[i].b = 255 - data[i].b;
    }
    return result;
}
inline Canvas Canvas::stitchHorizontal(const Canvas& rightSide) const{
    int newHeight = height;
    if (rightSide.height > newHeight){
        newHeight = rightSide.height;
    }
    Canvas result(width + rightSide.width, newHeight);
    result.paste(*this, 0, 0);
    result.paste(rightSide, width, 0);
    return result;
}
inline Canvas Canvas::stackVertical(const Canvas& bottomSide) const{
    int newWidth = width;
    if (bottomSide.width > newWidth){
        newWidth = bottomSide.width;
    }
    Canvas result(newWidth, height + bottomSide.height);
    result.paste(*this, 0, 0);
    result.paste(bottomSide, 0, height);
    return result;
}
inline Canvas Canvas::crop(int x, int y, int cropWidth, int cropHeight) const{
    Canvas result(cropWidth, cropHeight);
    for (int row = 0; row < result.height; row++){
        for (int col = 0; col < result.width; col++){
            int srcX = x + col;
            int srcY = y + row;
            if (srcX >= 0 && srcX < width && srcY >= 0 && srcY < height){
                result.at(col, row) = at(srcX, srcY);
            }
        }
    }
    return result;
}
inline void Canvas::paste(const Canvas& source, int destX, int destY){
    for (int sy = 0; sy < source.height; sy++){
        for (int sx = 0; sx < source.width; sx++){
            long long dx = (long long)destX + sx;
            long long dy = (long long)destY + sy;
            if (dx >= 0 && dx < width && dy >= 0 && dy < height){
                at((int)dx, (int)dy) = source.at(sx, sy);
            }
        }
    }
}
inline void Canvas::paste(const Canvas& source, int destX, int destY,const Pixel& transparentKey){
    for (int sy = 0; sy < source.height; sy++){
        for (int sx = 0; sx < source.width; sx++){
            const Pixel& p = source.at(sx, sy);
            if (p.r == transparentKey.r && p.g == transparentKey.g && p.b == transparentKey.b){
                continue;
            }
            long long dx = (long long)destX + sx;
            long long dy = (long long)destY + sy;
            if (dx >= 0 && dx < width && dy >= 0 && dy < height){
                at((int)dx, (int)dy) = p;
            }
        }
    }
}
inline Canvas Canvas::flipHorizontal() const{
    Canvas result(width, height);
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            result.at(width - 1 - x, y) = at(x, y);
        }
    }
    return result;
}
inline Canvas Canvas::flipVertical() const{
    Canvas result(width, height);
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            result.at(x, height - 1 - y) = at(x, y);
        }
    }
    return result;
}
inline Canvas Canvas::rotate90Clockwise() const{
    Canvas result(height, width);
    for (int y = 0; y < result.height; y++){
        for (int x = 0; x < result.width; x++){
            result.at(x, y) = at(y, height - 1 - x);
        }
    }
    return result;
}
inline Canvas Canvas::applyFilter(const Kernel& kernel) const{
    return applyFilter(kernel.weights, kernel.size);
}
inline Canvas Canvas::applyFilter(const float* weights, int kernelSize) const{
    if (weights == nullptr || kernelSize < 1 || kernelSize % 2 == 0){
        return Canvas(*this);
    }
    int r = kernelSize / 2;
    Canvas result(width, height);
    for (int y = 0; y < height; y++){
        for (int x = 0; x < width; x++){
            float sumR = 0.0f;
            float sumG = 0.0f;
            float sumB = 0.0f;
            for (int i = 0; i < kernelSize; i++){
                for (int j = 0; j < kernelSize; j++){
                    int cx = clampInt(x + j - r, 0, width - 1);
                    int cy = clampInt(y + i - r, 0, height - 1);
                    float w = weights[i * kernelSize + j];
                    const Pixel& p = at(cx, cy);
                    sumR += w * p.r;
                    sumG += w * p.g;
                    sumB += w * p.b;
                }
            }
            float sums[3] = { sumR, sumG, sumB };
            uint8_t out[3];
            for (int c = 0; c < 3; c++){
                if (!(sums[c] >= 0.0f)){         
                    out[c] = 0;
                }
                else if (sums[c] > 255.0f){
                    out[c] = 255;
                }
                else{
                    out[c] = (uint8_t)(sums[c] + 0.5f);
                }
            }
            result.at(x, y).r = out[0];
            result.at(x, y).g = out[1];
            result.at(x, y).b = out[2];
        }
    }
    return result;
}
inline int Canvas::getWidth() const{
    return width;
}
inline int Canvas::getHeight() const{
    return height;
}
inline const Pixel* Canvas::getRawData() const{
    return data;
}
#endif 
