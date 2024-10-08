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
 * @file cssparse.c
 * @author 350137278@qq.com
 * @brief A simple css file parser
 *
 * @version 0.0.7
 * @since     2024-10-07
 * @date 2024-10-09 01:33:44
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "cssparse.h"
#include "smallregex.h"

#define CSS_STRING_MAXSIZE   0xFFFF
#define CSS_KEYVAL_MAXSIZE   0x00FF

#ifdef _DEBUG
#   define DEBUG_ASSERT(cond)  assert((cond));
#else
#   define DEBUG_ASSERT(cond)  ;
#endif


struct CssKeyField {
    unsigned int flags:   4;    // 程序定义的状态位, 如: highlight  deleted mousemovein ...
    unsigned int type:    4;
    unsigned int length:  8;
    unsigned int offset: 16;
};


static void setCssKeyField(const char *cssString, struct CssKeyField *keyField, CssKeyType keytype, char *begin, int length)
{
    char *end = begin + length - 1;

    // 剔除头部无效字符
    while(length > 0) {
        if (*begin == ':' || *begin == ';' || *begin == ' ') {
            begin++;
            length--;
            continue;
        }
        break;
    }

    // 剔除尾部无效字符
    while(length > 0) {
        if (*end == ';' || *end == '}' || *end == ' ') {
            end--;
            length--;
            continue;
        }
        break;
    }

    if (length >= CSS_KEYVAL_MAXSIZE) {
        printf("Error: css key has too many chars: %s\n", begin);
        abort();
    }

    keyField->offset = (unsigned int)(begin - cssString);
    keyField->length = (unsigned int) length;
    keyField->type = (unsigned int) keytype;
}


CssKeyArray CssKeyArrayNew(int num)
{
    CssKeyArray keys = (CssKeyArray) malloc(sizeof(struct CssKeyField) * num);
    if (! keys) {
        printf("Error: out of memory\n");
        abort();
    }
    return keys;
}


void CssKeyArrayFree(CssKeyArray keys)
{
    if (keys) {
        free(keys);
    }
}


void CssStringFree(CssString cssString)
{
    if (cssString) {
        free(cssString);
    }
}


char * CssParseString(char *cssString, CssKeyArray outKeys, int *numKeys)
{
    int p, q, len;
    char tmpChar, *markStr;

    char *begin, *end, *start, *next;
    int keys = 0;

    char *css = cssString;
    int cssLen = (int) strnlen(cssString, CSS_STRING_MAXSIZE);

    if (cssLen == CSS_STRING_MAXSIZE) {
        printf("Error: cssString has too many chars\n");
        // 错误返回空
        return 0;
    }

    while (*css) {
        tmpChar = *css;
        if (tmpChar == 9 || tmpChar == 13 || tmpChar == 34 || tmpChar == 39) {
            // 用空格替代字符: [\t \r " ']
            *css = 32;  // space
        } else if (tmpChar == 10) {
            // 用 ; 替代换行符: [\n]
            *css = 59;
        }
        css++;
    }

    // 用空格替换注释: "/* ... */"
    struct small_regex * recomment = regex_compile("/\\*.*?\\*/");
    {
        css = cssString;
        while (*css) {
            p = regex_matchp(recomment, css);
            if (p < 0) {
                break;
            }
            start = css + p;
            next = strstr(start, "*/") + 2;
            len = (unsigned int)(next - start);

            while(len-- > 0) {
                *start++ = 32;
            }
            css = next;
        }
    }
    regex_free(recomment);

    // 查找每个属性集: "{ key: value; ... }"
    struct small_regex * reclass = regex_compile("{.*?}");
    struct small_regex * rekey = regex_compile(":.*?;");
    {
        css = cssString;
        while (*css) {
            p = regex_matchp(reclass, css);
            if (p < 0) {
                break;
            }

            start = css + p;
            DEBUG_ASSERT(*start == '{')

            next = strchr(start, '}') + 1;
            len = (unsigned int)(next - start);

            // 获取选择器名
            CssKeyType keytype = css_type_none;
            begin = css;
            while (begin < start) {
                // 选择器名只处理 3 种情况:
                if (*begin == '.') {
                    keytype = css_type_class;
                } else if (*begin == '#') {
                    keytype = css_type_id;
                } else if (*begin == '*') {
                    keytype = css_type_asterisk;
                }

                if (keytype) {
                    break;
                }

                begin++;
                p--;
            }

            if (p > 0) {
                // 如果发现选择器
                if (outKeys && keys < *numKeys) {
                    setCssKeyField(cssString, &outKeys[keys], keytype, begin, p);
                }
                keys++;

                markStr = start + len - 1;
                DEBUG_ASSERT(*markStr == '}')
                *markStr = ';';
                tmpChar = *next; *next = '\0';

                start++;
                while (start < next) {
                    // 查找属性: key : value ;
                    q = regex_matchp(rekey, start);
                    if (q < 0) {
                        break;
                    }

                    begin = start + q;
                    end = strchr(begin, ';') + 1;
                    len = (unsigned int)(end - begin);

                    DEBUG_ASSERT(*begin == ':')
                    DEBUG_ASSERT(begin[len - 1] == ';')

                    // set key
                    if (outKeys && keys < *numKeys) {
                        setCssKeyField(cssString, &outKeys[keys], css_type_key, start, q);
                    }
                    keys++;

                    // set value
                    if (outKeys && keys < *numKeys) {
                        setCssKeyField(cssString, &outKeys[keys], css_type_value, begin, len);
                    }
                    keys++;

                    start = end;
                }

                DEBUG_ASSERT(*markStr == ';')
                *markStr = '}';

                DEBUG_ASSERT(*next == '\0')
                *next = tmpChar;
            }

            // go to next braces: {}
            css = next;
        }
    }
    regex_free(rekey);
    regex_free(reclass);

    // 用户必须判断返回的 keys 不能 > 输入的 numKeys
    if (keys > *numKeys) {
        // 负值表示输入的空间不够, 其绝对值为需要的空间大小
        *numKeys = -keys;
    } else {
        // 正值表示正确
        *numKeys = keys;
    }

    return cssString;
}


CssString CssParseFile(FILE *cssFile, CssKeyArray outKeys, int *numKeys)
{
    rewind(cssFile);
    fseek(cssFile, 0, SEEK_END);
    int bsize = (int) ftell(cssFile);
    rewind(cssFile);

    if (bsize >= 0x03FFFF) {
        printf("Error: css file is too big\n");
        return 0;
    }

    char *cssString = (char *) malloc(bsize + sizeof(char));
    fread(cssString, sizeof(char), bsize, cssFile);
    cssString[bsize] = '\0';

    char * cssOut = CssParseString(cssString, outKeys, numKeys);
    if (! cssOut) {
        // 解析发生错误, 释放内存
        CssStringFree(cssString);
    }
    // 如果返回非空, 则用户使用完负责释放空间: CssStringFree(cssOut)
    return cssOut;
}


void CssPrintKeys(const CssString cssString, const CssKeyArray cssKeys, int numKeys)
{
    int flag = 1;

    for (int i = 0; i < numKeys; i++) {
        const char *key = cssString + cssKeys[i].offset;
        int keylen = cssKeys[i].length;

        switch (cssKeys[i].type) {
        case css_type_class:
        case css_type_id:
        case css_type_asterisk:
            if (! flag) {
                printf("}\n");
            }
            printf("<%.*s>{\n", keylen, key);
            flag = 1;
            break;

        case css_type_key:
            printf("  [%.*s]=", keylen, key);
            flag = 0;
            break;
        case css_type_value:
            printf("[%.*s]\n", keylen, key);
            flag = 0;
            break;

        default:
            abort();
            break;
        }
    }

    if (! flag) {
        printf("}\n");
    }
}