/*
*******************************************************************************
    ChenKe404's font library
*******************************************************************************
@project	ckfont
@authors	chenke404
@file	drawer.cpp
@brief 	font drawer abstruct class source

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chenke404
******************************************************************************
*/


#include "drawer.h"

namespace ck
{

// 返回空白字符占用的空格数
inline static int whitespace(const Font::Char& chr)
{
    if (chr.code == ' ') return 1;
    if (chr.code == '\t') return 2;
    return 0;
}

inline static void draw_line(
    const FontDrawer* drawer,
    Font::CharPtrList::const_iterator begin,
    Font::CharPtrList::const_iterator end,
    int x, int y,
    const FontDrawer::Line &line,
    int spacingX, int wsp,
    FontDrawer::Box* out_box = nullptr
    )
{
    const auto size = std::distance(begin,end);
    auto getchr = [size,&begin](int i) -> const Font::Char& {
        return **(begin + (i % size));
    };

    int cx = x + line.ox;
    int cy = y + line.oy;
    for(int i=line.left; i<line.right; ++i)
    {
        const auto& c = getchr(i);
        const auto sp = whitespace(c) * wsp;
        if(sp != 0)
            cx += sp;
        else
        {
            // cx += c.xoffset;
            drawer->perchar(cx + c.xoffset, cy + c.yoffset, &c, drawer->font()->getData(c));
            cx += c.xadvance + spacingX;
        }
    }
    if (out_box)
    {
        out_box->x = x + line.ox;
        out_box->y = y + line.oy;
        out_box->w = cx - out_box->x;
        out_box->h = drawer->font()->header().lineHeight;
    }
}

void FontDrawer::setFont(const Font * fnt)
{ _font = fnt; }

const Font *FontDrawer::font() const
{ return _font; }

void FontDrawer::setMixColor(color argb)
{ _mix = argb; }

color FontDrawer::mixColor() const
{ return _mix; }

FontDrawer::Box FontDrawer::measure(
    CharPtrList::const_iterator begin,
    CharPtrList::const_iterator end,
    int w, int h,
    const Options &opts,
    Lines* out_lines
    ) const
{
    const auto size = std::distance(begin,end);
    if(!_font || size < 1)
        return { 0,0,0,0 };
    const auto align = opts.align;
    const auto& header = _font->header();
    const int spc_x = opts.spacingX < 0 ? header.spacingX : opts.spacingX;
    const int spc_y = opts.spacingY;
    const auto lineHeight = header.lineHeight;
    const auto unBreakWord = !opts.breakWord;   // 是否不打断单词
    const int wsp = _font->c(' ').xadvance;        // 空格宽度

    const auto padding = _font->header().padding;
    auto getchr = [size,&begin](int i) -> const Font::Char& {
        return **(begin + (i % size));
    };

    // 计算文本宽高
    int textWidth = 0, textHeight = 0;
    {
        int lineWidth = 0;
        Line line;
        for(int i=0; i<=size; ++i)
        {
            const auto& c = getchr(i);
            if(c.code == '\0')
                continue;
            if(line.left < 0)
                line.left = i;

            const auto sp = whitespace(c) * wsp;
            const auto cw = (sp == 0) ? c.xadvance : sp;  // 字符宽度
            if((w >= 0 && lineWidth > 0 && lineWidth + cw > w) || c.code == '\n' || i == size)
            {
                bool skip = c.code == '\n' || sp != 0;  // 是否跳过当前字符
                if(!skip && unBreakWord && i < size)
                {
                    // 如果在此换行会打断单词, 则在上一个空格处换行,
                    // 如果上一处空格在行的一半之前则仍然打断单词(为了防止中文长句换行太难看)
                    int lw = lineWidth;
                    int idx = -1;
                    const int left = line.left + (i-1 - line.left) / 2;   // 空格在后半部分才换到下一行
                    for(int j=i-1; j>left; --j)
                    {
                        const auto& it = getchr(j);
                        const auto sp = whitespace(it) * wsp;
                        if(sp == 0)
                            lw -= c.xadvance + spc_x;
                        else
                        {
                            lw -= sp;
                            idx = j;    // 此处是空格
                            if(j-1 > 0 && getchr(j-1).code!=' ') // 前面不是空格则再减去一个间隔
                                lw -= spc_x;
                            break;
                        }
                    }
                    if(idx == -1)
                        lineWidth -= spc_x;
                    else
                    {
                        skip = true;    // 跳过当前空格
                        i = idx;
                        lineWidth = lw;
                    }
                }
                else
                    lineWidth -= spc_x;

                lineWidth += padding[0] + padding[1];   // 文本行宽+水平内间距
                line.right = i;
                line.width = lineWidth;
                if(out_lines)
                    out_lines->push_back(line);
                line.left = i;

                textWidth = std::max(textWidth,lineWidth);
                textHeight += lineHeight + spc_y;
                lineWidth = 0;
                if (skip)
                {
                    line.left = -1;
                    continue;
                }
            }
            if(sp == 0)  // 空格忽略间隔
                lineWidth += cw + spc_x;
            else
                lineWidth += sp;
        }

        textHeight -= spc_y;
        textHeight += padding[1] + padding[3]; // // 文本高+垂直内间距
    }

    // 计算文本偏移
    int ox=0,oy=0;
    {
        if(align & AL_RIGHT)
            ox -= (w > 0) ? textWidth - w : textWidth;
        else if(align & AL_HCENTER)
            ox -= (w > 0) ? (textWidth - w) / 2 : textWidth / 2;

        if(align & AL_BOTTOM)
            oy -= (h > 0) ? textHeight - h : textHeight;
        else if(align & AL_VCENTER)
            oy -= (h > 0) ? (textHeight - h) / 2 : textHeight / 2;
    }

    if(out_lines)
    {
        int num = 0;
        for(auto& it : *out_lines)
        {
            if(align & AL_RIGHT)
                it.ox = ox + (textWidth - it.width);
            else if(align & AL_HCENTER)
                it.ox = ox + (textWidth - it.width) / 2;
            else
                it.ox = ox;
            it.oy = oy + (lineHeight + spc_y) * num;
            ++num;
        }
    }

    return { ox, oy, textWidth, textHeight };
}

FontDrawer::Box FontDrawer::draw(
    CharPtrList::const_iterator begin,
    CharPtrList::const_iterator end,
    int x, int y, int w, int h,
    const Options& opts
    ) const
{
    const auto size = std::distance(begin,end);
    if(!_font || size < 1)
        return { 0,0,0,0 };

    const auto& header = _font->header();
    const auto padding = header.padding;
    const int spc_x = opts.spacingX < 0 ? header.spacingX : opts.spacingX;
    const int wsp = _font->c(' ').xadvance;        // 空格宽度

    const auto ox = x + padding[0];
    const auto oy = y + padding[1];

    Lines lines;
    auto box = measure(begin,end,w,h,opts,&lines);
    for(auto& it : lines)
    {
        draw_line(this,begin,end,ox,oy,it,spc_x,wsp);
    }

    box.x += x;
    box.y += y;
    return box;
}

FontDrawer::Box FontDrawer::draw(
    const Font::CharList & chrs,
    int x, int y, int w, int h,
    const Options& opts
    ) const
{
    Font::CharPtrList ptrs;
    ptrs.resize(chrs.size());
    for(int i=0;i<chrs.size();++i)
    {
        ptrs[i] = &chrs[i];
    }
    return draw(ptrs.begin(),ptrs.end(), x, y, w, h, opts);
}

FontDrawer::Box FontDrawer::draw(
    const Font::CharPtrList& chrs,
    int x, int y,
    const Line &line,
    int spacingX
    )
{
    Box box{0,0,0,0};
    if(!_font) return box;
    auto& header = _font->header();
    const auto padding = header.padding;
    const int spc_x = spacingX < 0 ? header.spacingX : spacingX;
    const int wsp = _font->c(' ').width;
    draw_line(this,chrs.begin(),chrs.end(),x + padding[0],y + padding[1],line,spc_x,wsp,&box);
    return box;
}

void FontDrawer::draw(
    const Font::Char& chr,
    int x, int y
    )
{
    if(!_font) return;
    perchar(x + chr.xoffset, y + chr.yoffset, &chr, _font->getData(chr));
}

}
