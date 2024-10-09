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
 * @version 0.1.9
 * @since 2024-10-08 23:51:27
 * @date 2024-10-10 02:26:50
 */
#ifndef CSS_PARSE_H__
#define CSS_PARSE_H__

#if defined(__cplusplus)
extern "C"
{
#endif


typedef struct CssKeyField  *CssKeyArray, *CssKeyArrayNode;


#define CSS_STRING_MAXSIZE   0xFFFFF   // 20bit: 最长 1 M 字节
#define CSS_KEYINDEX_MAX     0xFFF     // 12bit: 最多 4095 个 Keys
#define CSS_VALUE_MAXLEN     0xFF      // 8bit: 键值的长度最大 255 个字符


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

extern int CssParseString(char* cssString, CssKeyArray outKeys);

extern int CssKeyArrayGetSize(const CssKeyArray cssKeys);
extern int CssKeyArrayGetUsed(const CssKeyArray cssKeys);

extern const CssKeyArrayNode CssKeyArrayGetNode(const CssKeyArray cssKeys, int index);
extern CssKeyType CssKeyGetType(const CssKeyArrayNode cssKey);
extern int CssKeyGetFlag(const CssKeyArrayNode cssKey);
extern int CssKeyOffsetLength(const CssKeyArrayNode cssKeyNode, int* bOffset);
extern int CssKeyTypeIsClass(const CssKeyArrayNode cssKeyNode);
extern int CssKeyFlagToString(int keyflag, char* outbuf, size_t buflen);

// 展示了如何使用 cssparse 解析输出 css
extern void CssKeyArrayPrint(const char *cssString, const CssKeyArray cssKeys, FILE *fpout);

#ifdef __cplusplus
}
#endif
#endif /* CSS_PARSE_H__ */