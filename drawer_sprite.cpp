#include "drawer_sprite.h"

namespace ck
{

////////////////////////////////////////
/// FontTexture

static constexpr FontTexture::Char LT { '\t' };
static constexpr FontTexture::Char L0 { '\0' };
static constexpr FontTexture::Char LN { '\n' };

template<typename C>
inline FontTexture::CharPtrList __cs(const FontTexture &f,const C* str)
{
    if (!str) return {};
    const auto len = std::char_traits<C>::length(str);
    FontTexture::CharPtrList ret;
    ret.resize(len);
    for(int i=0;i<(int)len;++i)
    {
        ret[i] = &f.c(str[i]);
    }
    return ret;
}

FontTexture::FontTexture()
{
    _sp.code = ' ';
}

const FontTexture::Char &FontTexture::c(char32_t c) const
{
    if (_chrs.empty() || c == '\r')
        return L0;
    if (c == '\n')
        return LN;
    if (c == '\t')
        return LT;
    auto iter = _map.find(c);
    if(iter == _map.end())
    {
        if(c == ' ')
            return _sp;
        else
            return _chrs.front();
    }
    return *iter->second;
}

FontTexture::CharPtrList FontTexture::cs(const char *str) const
{
    return __cs<char>(*this,str);
}

FontTexture::CharPtrList FontTexture::cs(const wchar_t *str) const
{
    return __cs<wchar_t>(*this,str);
}

FontTexture::CharPtrList FontTexture::cs(const char32_t *str) const
{
    return __cs<char32_t>(*this,str);
}

FontTexture::CharPtrList FontTexture::css(const std::string &str) const
{
     return __cs<char>(*this,str.c_str());
}

FontTexture::CharPtrList FontTexture::css(const std::wstring &str) const
{
    return __cs<wchar_t>(*this,str.c_str());
}

FontTexture::CharPtrList FontTexture::css(const std::u32string &str) const
{
    return __cs<char32_t>(*this,str.c_str());
}

void FontTexture::setCharset(const CharList &cs)
{
    _chrs = cs;
    _map.clear();
    for(auto& c : _chrs)
    {
        _map[c.code] = &c;
    }
}

void FontTexture::setCharset(CharList &&cs)
{
    _chrs = std::forward<CharList>(cs);
    _map.clear();
    for(auto& c : _chrs)
    {
        _map[c.code] = &c;
    }
}

const FontTexture::CharList &FontTexture::charset() const
{
    return _chrs;
}

void FontTexture::setPages(const std::vector<void *> &pages)
{
    _pages = pages;
}

void FontTexture::setPages(std::vector<void *> &&pages)
{
    _pages = std::forward<std::vector<void *>>(pages);
}

const std::vector<void *> &FontTexture::pages() const
{
    return _pages;
}

void FontTexture::clear()
{
    _map.clear();
    _chrs.clear();
    _pages.clear();
}

struct yoffset{
    uint32_t start = 0, end = 0;
    uint32_t y;
};
using yoffset_line = std::vector<yoffset>;

inline int find_yoffset(
    const yoffset_line& line,
    uint32_t start,uint32_t end
    )
{
    if(end < start)
        std::swap(start, end);
    uint32_t oy = 0;
    bool in_range = false;
    for(auto i=line.begin(); i!=line.end(); ++i)
    {
        if(start >= i->start && start <= i->end)
            in_range = true;
        if(in_range)
            oy = std::max(oy,i->y);
        if(end >= i->start && end <= i->end)
            break;
    }
    return oy;
}

FontTextureCreator::FontTextureCreator(
    uint32_t width,
    uint32_t height,
    uint8_t spacing
    )
    : _width(width),_height(height),_spacing(spacing)
{
    if(_spacing < 1)
        _spacing = 1;
}

bool FontTextureCreator::start(
    const Font * fnt,
    FontTexture& out
    )
{
    out.clear();
    const auto& chrs = fnt->chrs();
    CharList out_chrs;
    out_chrs.reserve(chrs.size());
    std::vector<void*> out_pages;

    int page = 0;
    uint32_t left = _spacing,top = _spacing;
    Char chr;
    yoffset_line yo_last{{ 0,_width,_spacing }};   // 上一行的y偏移
    yoffset_line yo_cur;    // 当前行的y偏移

    auto& it = chrs.back();

    void* texture = newTexture();
    for(auto i=chrs.rbegin(); i!=chrs.rend() && texture; ++i)
    {
        const auto& c = *i;
        if(c.width + _spacing * 2 > _width) // 跳过宽度+间隔直接大于纹理宽度的
            continue;
        if(c.height + _spacing * 2 > _height)   // 跳过高度+间隔直接大于纹理高度的
            continue;

        auto right = left + c.width + _spacing;
        if(right > _width)
        {
            left = _spacing;
            right = left + c.width + _spacing;
            yo_last = std::move(yo_cur);
        }

        top = find_yoffset(yo_last,left,right);
        auto bottom = top + c.height + _spacing;
        if(bottom > _height)
        {
            out_pages.push_back(texture);
            if(std::next(i) != chrs.rend())
                texture = newTexture();     // 新页
            else    // 有可能最后一页刚好放下最后一个字
                texture = nullptr;
            ++page;
            yo_last = {{ 0,_width,_spacing }};
            yo_cur.clear();
            top = _spacing;
            bottom = top + c.height + _spacing;
        }

        (Font::Char&)chr = c;
        chr.page = page;
        chr.x = left;
        chr.y = top;
        perchar(fnt,chr,fnt->getData(c),texture);
        out_chrs.emplace_back(chr);

        yo_cur.emplace_back(yoffset{left,right,bottom});
        left = right;
    }
    out_chrs.shrink_to_fit();
    if(!out_chrs.empty() && texture)    // 最后一页
        out_pages.push_back(texture);

    out.setCharset(std::move(out_chrs));
    out.setPages(std::move(out_pages));
    return !out_chrs.empty();
}

}
