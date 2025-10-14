/*
*******************************************************************************
    ChenKe404's font library
*******************************************************************************
@project	ckfont
@authors	chenke404
@file	font.h
@brief 	chenke404's font header

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chenke404
******************************************************************************
*/

#ifndef CK_FONT_H
#define CK_FONT_H

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace ck
{

using color = uint32_t;

inline color rgb(uint8_t r,uint8_t g,uint8_t b)
{ return (0xff << 24) + (r << 16) + (g << 8) + b; }

inline color argb(uint8_t a, uint8_t r,uint8_t g,uint8_t b)
{ return (a << 24) + (r << 16) + (g << 8) + b; }

// 颜色alpha分量
inline uint8_t ca(color c)
{ return uint8_t(c >> 24); }

// 颜色red分量
inline uint8_t cr(color c)
{ return uint8_t(c >> 16); }

// 颜色green分量
inline uint8_t cg(color c)
{ return uint8_t(c >> 8); }

// 颜色blue分量
inline uint8_t cb(color c)
{ return uint8_t(c); }

template<typename T>
inline uint8_t clamp(T v)
{
    v = round(v);
    return v > 0xFF ? 0xFF : (v < 0 ? 0 : (uint8_t)v);
}

// 混合颜色
// @bg 背景
// @fg 前景
// @mutiply 正片叠底
inline color mix(color bg, color fg, bool mutiply)
{
    const auto ra = ca(bg) * ca(fg) / 255.0;
    auto a = clamp(ca(bg)+ca(fg) - ra);

    auto fx1 = [](color bv,color fv) -> double{
        return bv;
    };
    auto fx2 = [](color bv,color fv) -> double{
        return (bv+fv - (bv*fv) / 255.0);
    };

    std::function<double(color,color)> fx = fx1;
    if(mutiply) fx = fx2;

    auto r = clamp((cr(bg)*ca(bg) + cr(fg)*ca(fg) - ra * fx(cr(bg),cr(fg))) / a);
    auto g = clamp((cg(bg)*ca(bg) + cg(fg)*ca(fg) - ra * fx(cg(bg),cg(fg))) / a);
    auto b = clamp((cb(bg)*ca(bg) + cb(fg)*ca(fg) - ra * fx(cb(bg),cb(fg))) / a);
    return argb(a,r,g,b);
}

struct Font
{
    enum Flag {
        FL_BIT32    = 1     // 是否是32位色, 带有alpha通道(只在创建时设置,后续不可更改)
    };
    struct Header
    {
        uint8_t lang[4];    // 语言标记
        uint8_t flag;       // 标志(备用)
        uint16_t count;     // 字符数量
        uint8_t lineHeight; // 字体行高
        uint8_t maxWidth;   // 最大字符宽度
        uint8_t spacingX;   // 推荐水平间距
        color transparent;  // 透明色值, 非32位色才有效
        uint8_t padding[4]; // 字符的内边距(left,top,right,bottom), 字体创建后就不能再更改
    };

    // 字符信息
    struct Char
    {
        char32_t code = 0;      //字符的unicode码
        uint32_t pos = 0;       //字符颜色数据的地址(插入时自动计算)
        uint8_t width = 0;		//字符宽度(包括水平内间距)
        uint8_t height = 0;		//字符高度(包括垂直内间距)
        uint8_t xadvance = 0;   //字符宽度(不包括水平间距)+字符间的固有间距
        int8_t xoffset = 0;		//字符水平偏移
        int8_t yoffset = 0;		//字符垂直偏移
    };
    using CharList = std::vector<Char>;
    using CharPtrList = std::vector<const Char*>;
    using fn_offset = uint32_t(*)(uint16_t x,uint16_t y,uint16_t w);
    using fn_to_color = color(*)(const uint8_t*);

    // 字符数据
    struct DataPtr;
    struct Data
    {
        Data();
        Data(const DataPtr&);
        Data(Data&&);
        inline uint8_t w() const { return _w; }
        inline uint8_t h() const { return _h; }

        const uint8_t* ptr() const;
        color get(int x,int y) const;
        bool valid() const;
        void operator=(const DataPtr&);
    private:
        friend struct Font;
        std::vector<uint8_t> _data;
        uint8_t _w,_h;
        fn_offset offset;
        fn_to_color to_color;
    };

    // 字符数据指针
    struct DataPtr
    {
        DataPtr();
        DataPtr(Data&);
        DataPtr(const Font* fnt,const uint8_t* ptr,uint8_t w,uint8_t h);
        inline uint8_t w() const { return _w; }
        inline uint8_t h() const { return _h; }

        const uint8_t* ptr() const;
        color get(int x,int y) const;
        bool valid() const;
    private:
        friend struct Data;
        const uint8_t* _ptr;
        uint8_t _w,_h;
        fn_offset offset;
        fn_to_color to_color;
    };

    // 字体适配器, 用于从其他格式读取字体
    struct Adapter
    {
        const Header& header() const;
        Header& header();
        const CharList& charList() const;
        const std::vector<uint8_t>& data() const;
    protected:
        Header _header;
        CharList _chrs;
        std::vector<uint8_t> _data;
    };

    Font();

    const Char& c(char32_t chr) const;
    CharPtrList cs(const char* str) const;
    CharPtrList cs(const wchar_t* str) const;
    CharPtrList cs(const char32_t* str) const;
    CharPtrList css(const std::string& str) const;
    CharPtrList css(const std::wstring& str) const;
    CharPtrList css(const std::u32string& str) const;

    const Header& header() const;
    void setHeader(const Header& header);

    // 返回字符列表
    const CharList& chrs() const;

    // 获取总图像数据
    const std::vector<uint8_t>& data() const;

    // 获取字符像素颜色
    color getColor(const Char& ch,int x,int y) const;

    // 获取字符像素颜色
    void getColor(const Char& ch,int x,int y,
                  uint8_t& out_r,uint8_t& out_g,uint8_t& out_b,uint8_t* out_a = nullptr
                  ) const;

    // 获取字符图像数据的指针访问对象
    DataPtr getData(const Char& ch) const;

    // 获取字符的图像数据
    bool getData(const Char& ch,Data& out) const;


    // 插入字符
    bool insert(const Char& ch,const Data& data);
    // 删除字符
    void remove(char32_t ch);
    // 清除所有字符
    void clear();

    // 读取字体文件
    bool open(const std::string& filename);
    // 保存字体文件
    bool save(const std::string& filename,bool compress = false);
    // 从适配器读取字体
    bool load(const Adapter&);
    // 从输入流读取字体
    bool load(std::istream& si);
    // 从内存读取字体
    bool load(const uint8_t* data,uint32_t size);
    // 当前字体是否有效
    bool valid() const;
private:
    template<typename Rd>
    friend bool load(Font&,Rd&);
    friend struct DataPtr;

    fn_offset offset;
    fn_to_color to_color;

    Header _header;
    std::unordered_map<char32_t,Char*> _map;
    CharList _chrs;
    std::vector<uint8_t> _data;
    Char _sp;   // 缺省空格字符
};

}

#endif // CK_FONT_H
