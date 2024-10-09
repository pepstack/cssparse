/**
 * Copyright © 2024 MapAware, Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * @file cssparse.h
 * @author mapaware@hotmail.com
 * @brief A simple css file parser
 *
 * @version 0.1.8
 * @since 2024-10-08 23:51:27
 * @date 2024-10-09 22:33:09
 *
 * file: <test.css>
 *
    .polygon {
        border-width: 3px;
        border-style: solid;
        border-color: #FFFF00;
        fill-opacity: 1;
        fill-style: solid;
        fill-color: #00FFFF;
    }

    .polygon selected {
        border-width: 3px;
        border-style: solid;
        border-color: #FF0F0F;
        fill-opacity: 1;
        fill-style: solid;
        fill-color: #00FFFF;
    }

    * {
        border-width: 4px;
        border-style: dash;
        border-color: #00FFF00;
        fill-opacity: 0;
        fill-style: solid;
        fill-color: #0011FF;
    }

    #123 {
        border-width: 5px;
        border-style: none;
        border-color: #AAFF00;
        fill-opacity: 10;
        fill-style: solid;
        fill-color: #00CCFF;
    }

 * Show how to parse css file:

void parse_print_cssfile(const char *cssFile)
{
    CssString cssString = 0;
    int numKeys = 0;
    CssKeyArray cssOutKeys = 0;

    FILE * fp = fopen(cssFile, "r");
    if (! fp) {
        printf("Warn: invalid css file: %s\n", cssFile);
        return;
    }

    // 测试空间以分配内存
    cssString = CssParseFile(fp, 0, &numKeys);
    fclose(fp);

    if (cssString && numKeys < 0) {
        numKeys = -numKeys;
        cssOutKeys = CssKeyArrayNew(numKeys);

        if (CssParseString(cssString, cssOutKeys, &numKeys) && numKeys > 0) {
            // 使用 cssOutKeys
            CssPrintKeys(cssString, cssOutKeys, numKeys);

            // TODO:
            // ...


        }

        CssKeyArrayFree(cssOutKeys);
    }
    CssStringFree(cssString);
}

Enjoy it!
 */
#ifndef CSS_PARSE_H__
#define CSS_PARSE_H__

#if defined(__cplusplus)
extern "C"
{
#endif

typedef char* CssString;
typedef struct CssKeyField* CssKeyArray;

typedef enum {
    css_type_none = 0,
    css_type_key = 1,
    css_type_value = 2,
    css_type_class = 46,    // '.' class
    css_type_id = 35,       // '#' id
    css_type_asterisk = 42  // '*' any
} CssKeyType;


// Max up to 16 Bits(Flags)
typedef enum {
    css_bitflag_none = 0,
    css_bitflag_readonly = 1,      // 只读 2^0
    css_bitflag_hidden = 2,        // 隐藏 2^1
    css_bitflag_hilight = 4,       // 掠过: mouse move in
    css_bitflag_pickup = 8,        // 拾取
    css_bitflag_dragging = 16,     // 拖动中
    css_bitflag_deleting = 32,     // 删除中
    css_bitflag_fault = 64,        // 错误
    css_bitflag_flash = 128,       // 闪烁
    css_bitflag_zoomin = 256,      // 视图窗口放大
    css_bitflag_zoomout = 512,     // 视图窗口缩小
    css_bitflag_panning = 1024     // 视图窗口移动
} CssBitFlag;


extern CssKeyArray CssKeyArrayNew(int num);

extern void CssKeyArrayFree(CssKeyArray keys);

extern char* CssParseString(char* cssString, CssKeyArray outKeys, int* numKeys);

extern CssString CssParseFile(FILE* cssFile, CssKeyArray outKeys, int* numKeys);

// CssString 类型释放器, 仅仅用于释放 CssParseFile() 返回的对象
extern void CssStringFree(CssString cssString);

// 方法展示了如何使用 cssparse 解析输出 css
///extern int CssKeysFindFirst(const CssString cssString, const CssKeyArray cssKeys, int numKeys);

extern void CssPrintKeys(const CssString cssString, const CssKeyArray cssKeys, int numKeys);

extern void CssKeyArrayPrint(const CssString cssString, const CssKeyArray cssKeys, int numKeys, FILE *fpout);


#ifdef __cplusplus
}
#endif
#endif /* CSS_PARSE_H__ */