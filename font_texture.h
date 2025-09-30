#ifndef CK_FONT_TEXTURE_H
#define CK_FONT_TEXTURE_H

#include "font.h"
// #include "cstdint"

namespace ck
{

class FontTexture
{
public:
    struct Char : public Font::Char
    {
        uint8_t page = 0;   // 字符所在的纹理页
        int x = 0, y = 0;   // 字符的起始像素坐标(左上角原点)
    };
    using CharList = std::vector<Char>;
    using CharPtrList = std::vector<const Char*>;
public:
    FontTexture();

    const Char& c(char32_t chr) const;
    CharPtrList cs(const char* str) const;
    CharPtrList cs(const wchar_t* str) const;
    CharPtrList cs(const char32_t* str) const;
    CharPtrList css(const std::string& str) const;
    CharPtrList css(const std::wstring& str) const;
    CharPtrList css(const std::u32string& str) const;

    const CharList& chrs() const;
    const std::vector<void*>& pages() const;

    void clear();
private:
    std::map<char32_t,Char*> _map;
    CharList _chrs;
    std::vector<void*> _pages;
    friend class FontTextureCreator;
};

// 创建字体纹理
class FontTextureCreator
{
public:
    using Char = FontTexture::Char;
    using CharList = std::vector<Char>;
public:
    FontTextureCreator(
        uint32_t width,     // 纹理宽
        uint32_t height,    // 纹理高
        uint8_t spacing     // 字符间隔
        );

    // 估算可以容纳下全部字符的正方形纹理的宽度
    static int estimate(
        const Font& fnt,
        uint8_t spacing
        );

    // 估算空间损耗最少得2次幂纹理尺寸
    static int estimate(
        const Font& fnt,
        uint8_t spacing,
        uint32_t min_width, // 限制最小大小<最小大小总是不会超过128像素>
        uint32_t max_width  // 限制最大大小<
        );

    // 开始转换
    bool start(
        const Font& fnt,  // 字体
        FontTexture& out
        );

    // 请求创建新纹理
    virtual void* newTexture() = 0;

    virtual void perchar(const Font& fnt,const Char&, const Font::DataPtr &d, void* texture) = 0;
protected:
    uint32_t _width, _height;
    uint8_t _spacing;
};

}

#endif // CK_FONT_DRAWER_SPRITE_H
