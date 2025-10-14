/*
*******************************************************************************
    ChenKe404's font library
*******************************************************************************
@project	ckfont
@authors	chenke404
@file	drawer.h
@brief 	font drawer abstruct class header

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chenke404
******************************************************************************
*/

#ifndef CK_FONT_DRAWER_H
#define CK_FONT_DRAWER_H

#include "font.h"

namespace ck
{

struct FontDrawer
{
    using CharPtrList = Font::CharPtrList;

    enum Align {
        AL_LEFT     = 1 << 1,
        AL_HCENTER  = 1 << 2,
        AL_RIGHT    = 1 << 3,
        AL_TOP      = 1 << 4,
        AL_VCENTER  = 1 << 5,
        AL_BOTTOM   = 1 << 6
    };

    struct Box{
        int x,y,w,h;

        inline void adjust(int x, int y, int w, int h) {
            this->x += x; this->y += y;
            this->w += w; this->h += h;
        };
    };

    struct Line
    {
        int left = -1;  // 开始字符索引
        int right = -1; // 结束字符索引
        int ox = 0;     // 绘制水平偏移
        int oy = 0;     // 绘制垂直偏移
        int width = 0;  // 行宽
    };
    using Lines = std::vector<Line>;

    struct Options
    {
        uint8_t align = AL_LEFT | AL_BOTTOM;    // 对齐方式
        int spacingX = -1;      // 水平间距; -1:使用字体推荐间距
        int spacingY = 0;       // 水平间距;
        //bool rightHandSystem = false;   // TODO: 是否使用右手坐标系统
        bool breakWord = true;  // 换行时是否分割单词
    };

    // 设置当前使用的字体
    virtual void setFont(const Font*);
    const Font* font() const;

    // 设置混合颜色, alpha通道表示颜色的混合强度
    void setMixColor(color argb);
    color mixColor() const;

    Box measure(
        CharPtrList::const_iterator begin,
        CharPtrList::const_iterator end,
        int w = -1, int h = -1,
        const Options& opts = {},
        Lines* out_lines = nullptr
        ) const;

    inline Box measure(
        const CharPtrList& chrs,
        int w = -1, int h = -1,
        const Options& opts = {},
        Lines* out_lines = nullptr
        ) const
    {
        return measure(chrs.begin(), chrs.end(), w, h, opts, out_lines);
    }

    // 执行绘制, 每个字符都会调用perchar函数, 在perchar处理具体的绘制操作
    // @x,y     绘制的开始位置
    // @w       绘制域的宽度; -1表示无限, 此时相对于原点对齐
    // @h       绘制域的高度
    // @out_box 输出文本的矩形框
    // @opts    文本绘制选项
    virtual Box draw(
        CharPtrList::const_iterator begin,
        CharPtrList::const_iterator end,
        int x, int y, int w = -1, int h = -1,
        const Options& opts = {}
        ) const;

    inline Box draw(
        const CharPtrList& chrs,
        int x, int y, int w = -1, int h = -1,
        const Options& opts = {}
        ) const
    {
        return draw(chrs.begin(), chrs.end(), x, y, w, h, opts);
    }

    virtual Box draw(
        const Font::CharList&,
        int x, int y, int w = -1,int h = -1,
        const Options& opts = {}
        ) const;

    // 绘制一行
    virtual Box draw(
        const CharPtrList&,
        int x,int y,
        const Line& line,
        int spacingX = -1
        );

    // 绘制一个字符
    virtual void draw(
        const Font::Char&,
        int x,int y
        );

    // @box 字符要绘制的位置和字符的宽高
    virtual void perchar(int x, int y, const Font::Char* chr, const Font::DataPtr& d) const = 0;
protected:
    const Font* _font = nullptr;
    color _mix = 0;
};

}

#endif // CK_FONT_DRAWER_H
