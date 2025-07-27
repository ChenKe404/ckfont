// /*******************************************************************************/
//                                        ChenKe404's font library
// /*******************************************************************************/
//  *  @project	ckfont
//  *  @authors	chenke404
//  *  @file		xx
//  *  @brief 	xx
//
// MIT License
//
// Copyright (c) 2025 chenke404
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//  /******************************************************************************/

#include "font.h"
#include "fnt_adapter.h"

int main()
{
    ck::Font fnt;
    fnt.load("./test1.fnt");

    ck::FntAdapter adp;
    adp.load("E:/Tools/BMFont/test1.fnt",0);

    ck::Font fnt1;
    fnt1.load(adp);

    fnt1.save("./test1.fnt");

    return 0;
}
