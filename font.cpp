/*
*******************************************************************************
    ChenKe404's font library
*******************************************************************************
@project	ckfont
@authors	chenke404
@file	font.cpp
@brief 	chenke404's font source

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chenke404
******************************************************************************
*/

#include "font.h"
#include <cstring>
#include <map>
#include <fstream>
#include <iostream>
#include <lz4xx.h>

using namespace lz4xx;

static inline void warning(const char* text)
{ std::cerr << "ck::Font [WARN] -> " << text << std::endl; }

namespace ck
{

using Header = Font::Header;
using Char = Font::Char;
using CharList = Font::CharList;
using CharPtrList = Font::CharPtrList;

static constexpr Char LT { '\t' };
static constexpr Char L0 { '\0' };
static constexpr Char LN { '\n' };

// 计算data起始地址
inline uint32_t address_start(const CharList& chrs)
{ return sizeof(Header) + (uint32_t)chrs.size() * sizeof(Char); }

color to_color_24(const uint8_t* p)
{ return rgb(*p,*(p+1),*(p+2)); }

color to_color_32(const uint8_t* p)
{ return argb(*p,*(p+1),*(p+2),*(p+3)); }

uint32_t offset_24(uint16_t x,uint16_t y, uint16_t w)
{ return (y * w + x) * 3; }

uint32_t offset_32(uint16_t x,uint16_t y, uint16_t w)
{ return (y * w + x) * 4; }

// 颜色位数
inline int bit(const Font::Header& h)
{ return h.flag & Font::FL_BIT32 ? 4 : 3; }

// 某字符数据的大小
inline uint32_t size_block(const Char& ch,int bit)
{ return ch.width * ch.height * bit; }

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Font
Font::Font()
{
    memset(&_header,0,sizeof(Header));
    _sp.code = ' ';
}

const Char &Font::c(char32_t c) const
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

template<typename C>
inline Font::CharPtrList __cs(const Font &f,const C* str)
{
    if (!str) return {};
    const auto len = std::char_traits<C>::length(str);
    CharPtrList ret;
    ret.resize(len);
    for(int i=0;i<(int)len;++i)
    {
        ret[i] = &f.c(str[i]);
    }
    return ret;
}

Font::CharPtrList Font::cs(const char *str) const
{
    return __cs<char>(*this,str);
}

CharPtrList Font::cs(const wchar_t *str) const
{
    return __cs<wchar_t>(*this,str);
}

CharPtrList Font::cs(const char32_t *str) const
{
    return __cs<char32_t>(*this,str);
}

Font::CharPtrList Font::css(const std::string &str) const
{
    return __cs<char>(*this,str.c_str());
}

CharPtrList Font::css(const std::wstring &str) const
{
    return __cs<wchar_t>(*this,str.c_str());
}

CharPtrList Font::css(const std::u32string &str) const
{
    return __cs<char32_t>(*this,str.c_str());
}

const Font::Header &Font::header() const
{
    return _header;
}

void Font::setHeader(const Header &header)
{
    int flag = 0;   // 不可更改的flag
    if(_header.flag & FL_BIT32)
        flag = FL_BIT32;
    // padding 不能更改
    uint8_t padding[4]{0};
    memcpy(padding,_header.padding,4);

    _header = header;
    _header.flag |= flag;
    _sp.width = std::max(_header.lineHeight / 2,2);
    _sp.height = _header.lineHeight;

    memcpy(_header.padding,padding,4);
}

const std::vector<uint8_t> &Font::data() const
{
    return _data;
}

color Font::getColor(const Char &ch, int x, int y) const
{
    if(_header.flag & FL_BIT32)
    {
        const auto i = ch.pos + (y * ch.width + x) * 4;
        return argb(_data[i],_data[i+1],_data[i+2],_data[i+3]);
    }
    else
    {
        const auto i = ch.pos + (y * ch.width + x) * 3;
        return rgb(_data[i],_data[i+1],_data[i+2]);
    }
}

void Font::getColor(const Char &ch, int x, int y, uint8_t &out_r, uint8_t &out_g, uint8_t &out_b,uint8_t* out_a) const
{
    if(_header.flag & FL_BIT32)
    {
        const auto i = ch.pos + (y * ch.width + x) * 4;
        out_r = _data[i+1];
        out_g = _data[i+2];
        out_b = _data[i+3];
        if(out_a) *out_a = _data[i];
    }
    else
    {
        const auto i = ch.pos + (y * ch.width + x) * 3;
        out_r = _data[i];
        out_g = _data[i+1];
        out_b = _data[i+2];
        if(out_a) *out_a = 0xff;
    }
}

Font::DataPtr Font::getData(const Char &ch) const
{
    auto iter = _map.find(ch.code);
    if(iter == _map.end())
        return { };
    const auto upper = ch.pos + ch.width * ch.height * bit(_header);
    if(upper > _data.size())
        return { };
    return { this, _data.data() + ch.pos, ch.width, ch.height };
}

bool Font::getData(const Char &ch, Data &out) const
{
    auto dp = getData(ch);
    if(!dp.valid()) return false;
    out = dp;
    out.offset = offset;
    out.to_color = to_color;
    return true;
}

const Font::CharList &Font::chrs() const
{
    return _chrs;
}

bool Font::insert(const Char &ch, const Data &data)
{
    if(ch.width < 1 || ch.height < 1)
    {
        warning("invalid character size!");
        return false;
    }
    auto& ref = data._data;
    if(ref.size() != size_block(ch,bit(_header)))
    {
        warning("unmatched character data size!");
        return false;
    }

    remove(ch.code);

    _chrs.push_back(ch);
    auto& it = _chrs.back();
    it.pos = _data.size();
    _data.insert(_data.end(),ref.begin(),ref.end());
    _map[it.code] = &it;

    _header.maxWidth = std::max(_header.maxWidth,ch.width);
    _header.count = (uint16_t)_chrs.size();

    return true;
}

void Font::remove(char32_t ch)
{
    auto iter = _map.find(ch);
    if(iter == _map.end())
        return;

    const auto& it = *iter->second;
    const auto pos = it.pos;
    const auto size = size_block(it,bit(_header));   // 字符data的大小
    // 删除data块
    _data.erase(_data.begin() + pos,_data.begin() + pos + size);
    // 删除map和vector元素
    _map.erase(iter);
    for(auto i=_chrs.begin(); i!=_chrs.end(); ++i)
    {
        if(i->code == ch)
        {
            _chrs.erase(i);
            break;
        }
    }
    // 其余字符的地址向前偏移
    for(auto& it : _chrs)
    {
        if(it.pos > pos)
            it.pos -= size;
    }
}

void Font::clear()
{
    memset((void*)&_header,0,sizeof(Header));
    _map.clear();
    _chrs.clear();
    _data.clear();
}

using ctx_compress = context<Compress>;
struct writer
{
    inline writer(std::ofstream* so)
        : _so(so),_ctx(nullptr)
    {}
    inline void attach(ctx_compress* ctx) {
        _so = nullptr;
        _ctx = ctx;
    }

    inline bool write(const uint8_t *data, size_t size){
        if(_ctx)
            return _ctx->update(data,size);
        else if(_so)
        {
            const auto before = _so->tellp();
            _so->write((char*)data,size);
            return _so->tellp() - before == size;    // 真实写入大小不能小于输入数据大小
        }
        return false;
    }

    template<typename T>
    inline bool write(const T* data,size_t size)
    {
        return write((uint8_t*)data,size);
    }

private:
    std::ofstream* _so = nullptr;
    ctx_compress* _ctx = nullptr;
};

bool Font::save(const std::string &filename,bool compress)
{
    std::ofstream fo(filename,std::ios::binary);
    if(!fo) return false;
    writer wt(&fo);
    // 写入"文件标签"和"压缩标志"
    wt.write("CKF",3);
    wt.write((char*)&compress,1);

    // 压缩
    writer_stream wts(fo);
    ctx_compress ctx;
    if(compress)
    {
        size_t contentSize = 0;
        contentSize += sizeof(Header);
        uint32_t sz_data = 0;
        for(auto& it : _chrs)
        {
            contentSize += sizeof(Char);
            sz_data += size_block(it,bit(_header));
        }
        contentSize += sz_data;
        ctx = std::move(lz4xx::compress(contentSize,wts));
        wt.attach(&ctx);
    }

    _header.count = (uint32_t)_chrs.size();
    _header.maxWidth = 0;
    for(auto& it : _chrs)
    {
        _header.maxWidth = std::max(_header.maxWidth,it.width);
    }
    // 写入文件头
    wt.write(&_header,sizeof(Header));
    // 写入字符信息
    uint32_t sz_data = 0;
    for(auto& it : _chrs)
    {
        wt.write(&it,sizeof(Char));
        sz_data += size_block(it,bit(_header));
    }
    // 写入字符数据
    _data.resize(sz_data);  // fit
    if(!wt.write(_data.data(),sz_data))
    {
        warning("the data has not been fully output!");
        return false;
    }

    if(compress)
        ctx.finish();

    fo.close();
    return true;
}

using ctx_decompress = context<Decompress>;
template<typename Rd>
struct reader : bio::ireader
{
    inline reader(Rd* rd) :
        _rd(rd),
        _rdb(nullptr)
    {}

    using Rdb = bio::reader<bio::Buffer>;
    inline void attach(Rdb* rdb) {
        _rd = nullptr;
        _rdb = rdb;
    }

    inline size_t read(uint8_t *data, size_t capacity) override {
        if(_rdb)
            return _rdb->read(data,capacity);
        else if(_rd)
            return _rd->read(data,capacity);
        return 0;
    }

    inline size_t seek(pos_t pos) override {
        if(_rdb)
            return _rdb->seek(pos);
        else if(_rd)
            return _rd->seek(pos);
        return 0;
    }

    inline size_t pos() const override {
        if(_rdb)
            return _rdb->pos();
        else if(_rd)
            return _rd->pos();
        return 0;
    }

    inline size_t offset(pos_t ofs) override
    {
        if(_rd) return _rd->offset(ofs);
        else if(_rdb) return _rdb->offset(ofs);
        return 0;
    }

    template<typename T>
    inline size_t read(T* data,size_t size)
    { return read((uint8_t*)data,size); }
private:
    Rd* _rd = nullptr;
    Rdb* _rdb = nullptr;
};

// 计算每个字符的地址是否在数据的范围之内, 以及data大小是否匹配
static bool validate(const std::vector<Char>& chrs, uint32_t size,int bit)
{
    uint32_t total = 0;  // 统计data应该的大小
    for(auto& it : chrs)
    {
        auto sz_block = size_block(it,bit);
        total += sz_block;
        if(it.pos + sz_block > size) // 判断地址溢出
            return false;
    }
    return total == size;
}

bool Font::load(const Adapter& adp)
{
    _chrs = adp.charList();
    _data = adp.data();
    _map.clear();

    _header = adp.header();
    _header.maxWidth = 0;
    for(auto& it : _chrs)
    {
        _header.maxWidth = std::max(_header.maxWidth,it.width);
    }
    if(_header.flag & FL_BIT32)
    {
        offset = offset_32;
        to_color = to_color_32;
    }
    else
    {
        offset = offset_24;
        to_color = to_color_24;
    }

    if(validate(_chrs,_data.size(),bit(_header)))
    {
        _chrs.shrink_to_fit();
        _data.shrink_to_fit();
        for(auto& it : _chrs)
        {
            _map[it.code] = &it;
        }
        // 缺省空格的宽度是行高的一半
        _sp.width = std::max(_header.lineHeight / 2,2);
        _sp.height = _header.lineHeight;
        return true;
    }
    else
    {
        warning("font validation failed!");
        _chrs.clear();
        _data.clear();
        return false;
    }
}

bool Font::load(const std::string &filename)
{
    std::ifstream fi(filename,std::ios::binary);
    if(!fi) return false;
    auto ret = load(fi);
    fi.close();
    return ret;
}

template<typename Rd>
static inline bool load(Font& that,Rd& _rd)
{
    auto size = _rd.seek(-1);
    _rd.seek(0);
    if(size < sizeof(Header) + 4)  // 至少有一个头大小
        return false;
    auto& map = that._map;
    auto& chrs = that._chrs;
    auto& data = that._data;
    auto& header = that._header;
    map.clear();
    chrs.clear();
    data.clear();

    reader<Rd> rd(&_rd);
    char tag[3];
    rd.read(tag,3);
    bool compressed = false;
    rd.read(&compressed,1);

    if(strncmp(tag,"CKF",3) != 0)
    {
        warning("illegal file tag!");
        return false;
    }

    buffer_t buf;
    reader_buffer rdb(&buf);
    if(compressed)
    {
        writer_buffer wtb(buf);
        lz4xx::decompress(rd,wtb);
        rd.attach(&rdb);
    }
    const auto beg = rd.pos();
    size = rd.seek(-1); // 重读大小
    rd.seek(beg);
    rd.read(&header,sizeof(Header));
    if(header.flag & Font::FL_BIT32)
    {
        that.offset = offset_32;
        that.to_color = to_color_32;
    }
    else
    {
        that.offset = offset_24;
        that.to_color = to_color_24;
    }

    if(header.count > 0)
    {
        // 读取字符信息
        chrs.reserve(header.count);
        Char c;
        uint32_t offset = sizeof(Header);
        int num = 0;
        while (num < header.count && offset < size)
        {
            rd.read((char*)&c,sizeof(Char));
            offset += sizeof(Char);
            chrs.push_back(c);
            ++num;
        }
        if(num != header.count)
        {
            warning("characters overflowed, maybe font was broken!");
            chrs.clear();
            return false;
        }

        // 读取字符图像数据
        const auto sz_data = size - rd.pos();
        if(sz_data > 0)
        {
            data.resize(sz_data);
            if(rd.read(data.data(),sz_data) < sz_data)
            {
                warning("the data has not been fully input!");
                return false;
            }
        }
    }
    if(validate(chrs,data.size(),bit(header)))
    {
        chrs.shrink_to_fit();
        data.shrink_to_fit();
        for(auto& it : chrs)
        {
            map[it.code] = &it;
        }
        // 缺省空格的宽度是行高的一半
        that._sp.width = std::max(header.lineHeight / 2,2);
        that._sp.height = header.lineHeight;
        return true;
    }
    else
    {
        warning("font validation failed!");
        chrs.clear();
        data.clear();
        return false;
    }
}

bool Font::load(std::istream &si)
{
    auto rd = make_reader(si);
    return ck::load(*this,rd);
}

bool Font::load(const uint8_t *data, uint32_t size)
{
    auto rd = make_reader(data,size);
    return ck::load(*this,rd);
}

bool Font::valid() const
{
    return !_chrs.empty();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// DataPtr
Font::DataPtr::DataPtr()
    : _ptr(nullptr),_w(0),_h(0),
    offset(nullptr),
    to_color(nullptr)
{}

Font::DataPtr::DataPtr(Data &data)
    : _ptr(data._data.data()),_w(data._w),_h(data._h),
    offset(data.offset),
    to_color(data.to_color)
{}

Font::DataPtr::DataPtr(const Font* fnt,const uint8_t *ptr, uint8_t w, uint8_t h)
    : _ptr(ptr),_w(w),_h(h),
    offset(fnt->offset),
    to_color(fnt->to_color)
{}

const uint8_t* Font::DataPtr::ptr() const
{
    return _ptr;
}

color Font::DataPtr::get(int x, int y) const
{
    if(!valid()) return 0;
    return to_color(_ptr + offset(x,y,_w));
}

bool Font::DataPtr::valid() const
{
    return _ptr != nullptr && _w > 0 && _h > 0 &&
           offset != nullptr && to_color != nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Data
Font::Data::Data()
    : _w(0),_h(0),
    offset(nullptr),
    to_color(nullptr)
{}

Font::Data::Data(const DataPtr &o)
    : _w(0),_h(0),
    offset(o.offset),
    to_color(o.to_color)
{
    (*this) = o;
}

Font::Data::Data(Data &&o)
    : _data(std::move(o._data)),
    _w(o._w),_h(o._h),
    offset(o.offset),
    to_color(o.to_color)
{
    o._w = 0;
    o._h = 0;
    o.offset = nullptr;
    o.to_color = nullptr;
}

const uint8_t* Font::Data::ptr() const
{
    return _data.data();
}

color Font::Data::get(int x, int y) const
{
    if(!valid()) return 0;
    return to_color(_data.data() + offset(x,y,_w));
}

bool Font::Data::valid() const
{
    return !_data.empty() && _w > 0 && _h > 0;
}

void Font::Data::operator=(const DataPtr &o)
{
    _data.clear();
    if(o.valid())
    {
        _w = o._w;
        _h = o._h;
        uint32_t size = 0;
        if(o.offset == offset_24)
            size = _w * _h * 3;
        else
            size = _w * _h * 4;
        _data.resize(size);
        memcpy(_data.data(),o._ptr,size);
    }
    _data.shrink_to_fit();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Adapter
const Font::Header &Font::Adapter::header() const
{
    return _header;
}

Font::Header &Font::Adapter::header()
{
    return _header;
}

const Font::CharList &Font::Adapter::charList() const
{
    return _chrs;
}

const std::vector<uint8_t> &Font::Adapter::data() const
{
    return _data;
}

}
