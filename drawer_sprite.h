#ifndef CK_FONT_DRAWER_SPRITE_H
#define CK_FONT_DRAWER_SPRITE_H

#include "drawer.h"

namespace ck
{

class FontTexture
{
public:
    struct Char : public Font::Char
    {
        uint8_t page;   // 字符所在的纹理页
        int x, y;       // 字符的起始纹理坐标(左上角原点)
    };
    using CharList = std::vector<Char>;
    using CharPtrList = std::vector<const Char*>;
public:
    const Char& c(char32_t chr) const;
    CharPtrList cs(const char* str) const;
    CharPtrList cs(const wchar_t* str) const;
    CharPtrList cs(const char32_t* str) const;
    CharPtrList css(const std::string& str) const;
    CharPtrList css(const std::wstring& str) const;
    CharPtrList css(const std::u32string& str) const;

    void setCharset(const CharList& cs);
    void setCharset(CharList&& cs);
    const CharList& charset() const;

    void setPages(const std::vector<void*>& pages);
    void setPages(std::vector<void*>&& pages);
    const std::vector<void*>& pages() const;

    void clear();
private:
    std::map<char32_t,Char*> _map;
    CharList _chrs;
    std::vector<void*> _pages;
    Char _sp;   // 缺省空格字符
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

    // 开始转换
    bool start(
        const Font* fnt,  // 字体
        FontTexture& out
        );

    // 请求创建新纹理
    virtual void* newTexture() = 0;

    virtual void perchar(const Font* fnt,const Char&, const Font::DataPtr &d, void* texture) const = 0;
protected:
    uint32_t _width, _height;
    uint8_t _spacing;
};

}

#endif // CK_FONT_DRAWER_SPRITE_H
