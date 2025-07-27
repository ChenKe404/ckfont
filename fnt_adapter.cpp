/*
*******************************************************************************
    ChenKe404's font library
*******************************************************************************
@project	ckfont
@authors	chenke404
@file	fnt_adapter.cpp
@brief 	BMFont *.fnt load adapter class source

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chenke404
******************************************************************************
*/

#include "fnt_adapter.h"
#include <fstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::string vstr(const std::string& line, const char* name)
{
    auto pos = line.find(name);
    if(pos == line.npos)
        return {};
    pos += strlen(name);
    // 跳过name后面的空格
    for(; pos < line.size() && line[pos]==' '; ++pos);
    // name后面必须是等于
    if(line[pos] != '=')
        return {};
    ++pos;
    // 跳过=后面的空格
    for(; pos < line.size() && line[pos]==' '; ++pos);
    const auto left = pos;
    for(; pos < line.size() && line[pos]!=' '; ++pos);
    const auto right = pos;
    auto s = line.substr(left,right-left);
    if(s.empty()) return {};
    if(s.front()=='"' || s.back()=='"') // 去除引号
    {
        s.pop_back();
        s.erase(s.begin());
    }
    return s;
}

int vint(const std::string& line, const char* name)
{
    auto s = vstr(line,name);
    if(s.empty()) return 0;
    return std::stoi(s);
}

void varray(const std::string& line, const char* name,int* out,uint8_t size)
{
    auto s = vstr(line,name);
    std::vector<std::string> sp;
    size_t left = 0, right = 0;
    while((right = s.find(',',left)) && left < right)
    {
        sp.push_back(s.substr(left,right-left));
        if(right == s.npos)
            break;
        left = right + 1;
    }
    size = std::min(size,(uint8_t)sp.size());
    for(int i=0;i<size;++i)
    {
        out[i] = std::stoi(sp[i]);
    }
}

namespace ck
{

struct Page
{
    stbi_uc* pixels;
    int w,h;    // 宽,高
    bool bit32;  // 是否32位色

    void copy(const FntAdapter::FntChar& c,std::vector<uint8_t>& out) const
    {
        const auto bit = bit32 ? 4 : 3;
        const auto size = c.width * c.height * bit;
        out.resize(size);
        int i=0;
        for(int y=c.y; y<(c.y+c.height); ++y)
        {
            for(int x=c.x; x<(c.x+c.width); ++x)
            {
                const auto pos = (y * w + x) * bit;
                if(bit32)
                {
                    // argb
                    out[i] = pixels[pos+3];
                    out[i+1] = pixels[pos];
                    out[i+2] = pixels[pos+1];
                    out[i+3] = pixels[pos+2];
                }
                else
                {
                    out[i] = pixels[pos];
                    out[i+1] = pixels[pos+1];
                    out[i+2] = pixels[pos+2];
                }
                i += bit;
            }
        }
    }
};

bool FntAdapter::load(const std::string &filename, color transparent, bool bit32)
{
    std::ifstream fi(filename);
    if (!fi) return false;

    _chrs.clear();
    _data.clear();

    struct
    {
        int size=0;
        int padding[4]{0};      // 上,右,下,左
        int spacing[2]{0,0};
        int lineHeight=0;
        int count=0;
        int scaleW=0;
        int scaleH=0;
        int pages=0;
    } info;
    std::vector<FntChar> chrs;
    std::vector<std::string> page_files;

    std::string line;
    auto vstr = [&line](auto name){ return ::vstr(line,name); };
    auto vint = [&line](auto name){ return ::vint(line,name); };

    FntChar c;
    while (std::getline(fi, line))
    {
        if (line.find("info ") == 0)
        {
            info.size = vint("size");     
            varray(line,"padding",info.padding,4);
            varray(line,"spacing",info.spacing,2);
        }
        else if (line.find("common ") == 0)
        {
            info.lineHeight = vint("lineHeight");
            info.scaleW = vint("scaleW");
            info.scaleH = vint("scaleH");
            info.pages = vint("pages");
        }
        else if (line.find("page ") == 0)
        {
            page_files.push_back(vstr("file"));
        }
        else if (line.find("chars ") == 0)
        {
            info.count = vint("count");
        }
        else if (line.find("char ") == 0)
        {
            c.code = vint("id");
            c.x = vint("x");
            c.y = vint("y");
            c.width = vint("width");
            c.height = vint("height");
            c.xoffset = vint("xoffset");
            c.yoffset = vint("yoffset");
            c.xadvance = vint("xadvance");
            c.page = vint("page");
            chrs.push_back(c);
        }
    }
    fi.close();

    memset((char*)&_header,0,sizeof(Font::Header));
    _header.count = info.count;
    _header.lineHeight = info.lineHeight;
    _header.transparent = transparent;
    _header.flag = bit32 ? Font::FL_BIT32 : 0;
    _header.padding[0] = info.padding[3];
    _header.padding[1] = info.padding[0];
    _header.padding[2] = info.padding[1];
    _header.padding[3] = info.padding[2];

    const auto path = filename.substr(0, filename.find_last_of("/\\") + 1);
    bool failed = false;
    std::vector<Page> pages;
    int channels;
    for(auto& p : page_files)
    {
        Page page;
        page.pixels = stbi_load((path + p).c_str(),&page.w,&page.h,&channels,bit32 ? STBI_rgb_alpha : STBI_rgb);
        page.bit32 = bit32;
        if(page.pixels)
            pages.push_back(page);
        else
        {
            std::cerr << "Font [WARN] -> Fnt adapter failed to load page! " << p << std::endl;
            failed = true;
            break;
        }

        /*
        std::ofstream fo("E:/Tools/BMFont/"+p+".rgb",std::ios::binary);
        if(fo)
        {
            auto remain = page.w * page.h * 3;
            auto size = 1024;
            int off = 0;
            char buf[1024];
            while(remain > 0)
            {
                if(remain < size)
                    size = remain;
                fo.write((char*)page.pixels + off,size);
                off += size;
                remain -= size;
            }
            fo.close();
        }
        */
    }

    if(info.pages != pages.size())
    {
        std::cerr << "Font [WARN] -> Fnt adapter loaded page count not match! ("<<
            pages.size() << "," << info.pages << ")" << std::endl;
        failed = true;
    }

    if(failed)
    {
        for(auto& it : pages)
        {
            stbi_image_free(it.pixels);
        }
        return false;
    }

    // 读取每个字符的图像数据
    uint32_t offset = 0;
    std::vector<uint8_t> buf;
    for(auto& ch : chrs)
    {
        const auto& page = pages[ch.page];
        page.copy(ch,buf);
        ch.pos = offset;
        _data.insert(_data.begin() + offset,buf.begin(),buf.end());
        _chrs.push_back(ch);
        offset += buf.size();
    }

    // 卸载图像
    for(auto& it : pages)
    {
        stbi_image_free(it.pixels);
    }
    return true;
}

}
