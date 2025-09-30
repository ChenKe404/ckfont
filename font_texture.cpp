#include "font_texture.h"
#include <algorithm>

namespace ck
{

// 比目标整数大的最小2次幂数
inline uint32_t number_pow2_greater(uint32_t v)
{
    uint32_t rt = 1;
    while(rt < v) {
        rt *= 2;
    }
    return rt;
}

// 比目标整数小的最大2次幂数
inline uint32_t number_pow2_lesser(uint32_t v)
{
    uint32_t rt = number_pow2_greater(v);
    auto lv = rt / 2;
    if(lv > 1)
        return lv;
    return rt;
}

// 取离目标整数最近的2次幂数
inline uint32_t number_pow2(uint32_t v)
{
    uint32_t rt = number_pow2_greater(v);
    auto lv = rt / 2;
    if(lv > 1 && v-lv < rt-v)
        return lv;
    return rt;
}

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
{}

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

const FontTexture::CharList &FontTexture::chrs() const
{
    return _chrs;
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

////////////////////////////////////////
/// FontTextureCreator

class __EstimateFontTextureCreator : public FontTextureCreator
{
public:
    struct info
    {
        int width = 0;
        int count = 0;  // 页数
        int remain_area = 0;    // 最后一页剩下的面积
        float remain_ratio = 0;   // 最后一页剩下面积的比例
    };
public:
    using FontTextureCreator::FontTextureCreator;
    void *newTexture() override {
        inf.count++;
        inf.remain_area = 0;
        inf.remain_ratio = 0;
        return (void*)0xffffffff;
    }
    void perchar(const Font &, const Char & c, const Font::DataPtr &, void *) override {
        inf.remain_area += (c.width + _spacing) * (c.height + _spacing);
    }

    info inf;
};

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

int FontTextureCreator::estimate(
    const Font &fnt,
    uint8_t spacing
    )
{
    auto increment = fnt.header().maxWidth;
    int width = 0;  // 起始宽度
    {
        uint64_t area = 0;
        for(auto& it : fnt.chrs())
        {
            area += (it.width + spacing) * (it.height + spacing);
        }
        width = int(sqrt(area));
    }
    if(width < 1)
        return 0;

    int last_n = 0;
    FontTexture ft;
    while (true){
        __EstimateFontTextureCreator eftc(width,width,spacing);
        eftc.start(fnt,ft);
        const auto n = ft._pages.size();
        if(n < 1)
            return 0;

        if(last_n > 1 && n == 1)    // 这次能一页放下
        {
            if(increment < 2)
                return width;
            else
                increment /= 2;
        }
        else if(last_n == 1 && n != 1 && increment > 2)  // 上次能一页放下
            increment /= 2;

        if(n > 1)
            width += increment;
        else
            width -= increment;

        last_n = n;
    }

    return width;
}

int FontTextureCreator::estimate(
    const Font &fnt,
    uint8_t spacing,
    uint32_t min_width,
    uint32_t max_width
    )
{
    using info = __EstimateFontTextureCreator::info;

    if(max_width < min_width)
        std::swap(min_width, max_width);
    min_width = number_pow2(min_width);
    max_width = number_pow2(max_width);

    FontTexture ft;
    std::vector<info> infos;
    for(auto w = min_width; w <= max_width; w *= 2)
    {
        __EstimateFontTextureCreator eftc(w,w,spacing);
        eftc.start(fnt,ft);
        eftc.inf.width = w;
        eftc.inf.remain_area = w * w - eftc.inf.remain_area;
        eftc.inf.remain_ratio = eftc.inf.remain_area / double(w * w);
        infos.push_back(eftc.inf);
    }

    std::sort(infos.begin(),infos.end(),[](auto& a,auto& b){
        return a.remain_ratio < b.remain_ratio;
    });

    if(!infos.empty())
        return infos.front().width;
    return min_width;
}

bool FontTextureCreator::start(
    const Font & fnt,
    FontTexture& out
    )
{
    out.clear();
    const auto& chrs = fnt.chrs();
    out._chrs.reserve(chrs.size());

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
            out._pages.push_back(texture);
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
        perchar(fnt,chr,fnt.getData(c),texture);
        out._chrs.emplace_back(chr);

        yo_cur.emplace_back(yoffset{left,right,bottom});
        left = right;
    }
    out._chrs.shrink_to_fit();
    if(!out._chrs.empty() && texture)    // 最后一页
        out._pages.push_back(texture);

    out._map.clear();
    for(auto& c : out._chrs)
    {
        out._map[c.code] = &c;
    }

    return !out._chrs.empty();
}

}
