/*
*******************************************************************************
    ChenKe404's font library
*******************************************************************************
@project	ckfont
@authors	chenke404
@file	fnt_adapter.h
@brief 	BMFont *.fnt load adapter class header

// SPDX-License-Identifier: MIT
// Copyright (c) 2025 chenke404
******************************************************************************
*/

#ifndef CK_FNT_ADAPTER_H
#define CK_FNT_ADAPTER_H

#include "font.h"

namespace ck
{

// BMFont
struct FntAdapter : public Font::Adapter
{
    struct FntChar : public Font::Char
    {
        int x = 0;
        int y = 0;
        int page = 0;
    };
    // @bit32: 是否是32位颜色(带alpha通道), 为true时transparent无效,
    //         如果原始图像是24位色, 那么alpha值将始终为255
    bool load(const std::string &filename, color transparent, bool bit32 = false);
};

}

#endif // CK_FNT_ADAPTER_H
