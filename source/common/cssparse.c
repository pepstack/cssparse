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
 * @version 0.0.9
 * @since     2024-10-07
 * @date 2024-10-09 23:31:26
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "cssparse.h"
#include "smallregex.h"

#define CSS_STRING_MAXSIZE   0xFFFFF   // 20bit: 最长 1 M 字节
#define CSS_KEYINDEX_MAX     0xFFF     // 12bit: 最多 4095 个 Keys
#define CSS_VALUE_MAXLEN     0xFF      // 8bit: 键值的长度最大 255 个字符



#ifdef _DEBUG
#   define DEBUG_ASSERT(cond)  assert((cond));
#else
#   define DEBUG_ASSERT(cond)  ;
#endif


#define CssCheckNumKeys(keys)  do { \
        if (keys >= CSS_KEYINDEX_MAX) { \
            printf("Error: Keys is too many (more than %d).\n", CSS_KEYINDEX_MAX); \
            abort(); \
        } \
    } while(0)


static const char* css_bitflag_array[] = {
    "readonly",
    "hidden",
    "hilight",
    "pickup",
    "dragging",
    "deleting",
    "fault",
    "flash",
    "zoomin",
    "zoomout",
    "panning",
    0
};


struct CssKeyField {
    unsigned int flags  : 16;  // 16 个标记位
    unsigned int type   :  8;  // 类型
    unsigned int length :  8;  // 名称长度

    unsigned int offset : 20;  // <= 0xFFFFF
    unsigned int keyidx : 12;  // 如果当前是 class, 这个值为定义 class 的 {key:value ...} 的索引位置;
                               // 如果当前是 keyval, 这个值为定义索引倒序
};


static int cssKeyTypeIsClass(char keytype)
{
    return (keytype == css_type_class || keytype == css_type_id || keytype == css_type_asterisk) ? 1 : 0;
}

static int cssGetKeyBitFlag(const char* classflag, int flaglen)
{
    if (flaglen < 4 || flaglen > 10) {
        return css_bitflag_none;
    }

    for (int i = 0; css_bitflag_array[i] != 0; i++) {
        if (!strncmp(css_bitflag_array[i], classflag, flaglen)) {
            return (CssBitFlag)(1<<i);
        }
    }

    return css_bitflag_none;
}


static int cssKeyBitFlagToString(int keyflag, char* outbuf, size_t buflen)
{
    if (keyflag > 0) {
        if (!outbuf) {
            int outlen = 0;
            for (int i = 0; css_bitflag_array[i] != 0; i++) {
                int bitmask = (1 << i);
                if (keyflag & bitmask) {
                    outlen += (int)strlen(css_bitflag_array[i]) + 1;
                }
            }
            return outlen + 1;
        }
        else {
            char* bbuf = outbuf;
            for (int i = 0; css_bitflag_array[i] != 0; i++) {
                int bitmask = (1 << i);
                if (keyflag & bitmask) {
                    int len = snprintf(bbuf, buflen + outbuf - bbuf, "%s ", css_bitflag_array[i]);
                    if (len <= 0) {
                        printf("Error: out of memory.\n");
                        abort();
                    }
                    bbuf += len;
                }
            }
            *bbuf = 0;
            return (int)(bbuf - outbuf);
        }
    }

    *outbuf = 0;
    return 0;
}


int cssParseClassFlags(const char* start, int len, int offsets[], int lengths[], int classflags[], int nsize)
{
    const char* p = start;
    const char* end = start + len;
    const char* str = 0;
    int n = 0;

    while (len-- > 0 && n < nsize) {
        if (*p == ' ' || *p == ',' || *p == '|' || *p == '\0') {
            if (str) {
                offsets[n] = (int)(str - start);
                lengths[n] = (int)(p - str);
                n++;
                str = 0;
            }
        }
        else if (!str) {
            str = p;
        }

        p++;
    }

    if (str) {
        offsets[n] = (int)(str - start);
        lengths[n] = (int)(p - str);
        n++;
    }

    /**
    * .a .b C D E  {...} -- both class a and b have all props C D E
    * .a, .b c d e {...} -- only class b has props C D E
    * .a c .b d e  {...} -- class a has all props, class b has D E
    * .a c, .b d e {...} -- class a has C, class b has D E
    */

    for (int i = 0; i < n; i++) {
        // 从开始取类名称
        int off0 = offsets[i];
        int len0 = lengths[i];

        // -1 表示 key 不是 class
        int classflag = -1;

        str = start + off0;
        CssKeyType keytype = str[0];

        if (cssKeyTypeIsClass(keytype)) {
            // 0 表示 key 是 class
            classflag = 0;

            // 如果是类, 查找后面逗号第一次出现的位置 stop
            const char *stop = str + len0;
            while (stop != end && *stop != ',') {
                stop++;
            }

            // 查找后面的 key
            for (int j = i + 1; j < n; j++) {
                int koff = offsets[j];
                int klen = lengths[j];

                if (koff < stop - start) {
                    // 逗号前面所有的 key
                    p = start + koff;

                    if (! cssKeyTypeIsClass(*p)) {
                        classflag |= cssGetKeyBitFlag(p, klen);
                    }
                }
            }
        }

        // 只需要取出 classflags[i] >= 0 的就是 class
        // >= 0: is bitflag of class
        //  < 0: is class its self
        classflags[i] = classflag;
    }

    return n;
}


static int setCssKeyField(const char* cssString, struct CssKeyField* keyField, CssKeyType keytype, char* begin, int length)
{
    int outkeys = 0;

    char* end = begin + length - 1;

    // 剔除头部无效字符
    while (length > 0) {
        if (*begin == ':' || *begin == ';' || *begin == ' ') {
            begin++;
            length--;
            continue;
        }
        break;
    }

    // 剔除尾部无效字符
    while (length > 0) {
        if (*end == ';' || *end == '}' || *end == ' ') {
            end--;
            length--;
            continue;
        }
        break;
    }

    if (cssKeyTypeIsClass(keytype)) {
        int offsets[256];
        int lengths[256];
        int keyflags[256];

        int numFlags = cssParseClassFlags(begin, length, offsets, lengths, keyflags, 256);

        for (int k = 0; k < numFlags; k++) {
            const char* classkey = begin + offsets[k];
            int keyflag = keyflags[k];
            if (keyflag >= 0) {
                if (lengths[k] < CSS_VALUE_MAXLEN) {
                    if (keyField) {
                        keyField[outkeys].type = keytype;
                        keyField[outkeys].flags = keyflag;
                        keyField[outkeys].offset = (int)(classkey - cssString);
                        keyField[outkeys].length = lengths[k];
                    }
                    outkeys++;
                }
                else {
                    printf("Error: css key has too many chars: %.*s\n", lengths[k], classkey);
                    abort();
                }
            }
        }
    }
    else {
        if (length < CSS_VALUE_MAXLEN) {
            if (keyField) {
                keyField->offset = (unsigned int)(begin - cssString);
                keyField->length = (unsigned int)length;
                keyField->type = (unsigned int)keytype;
                keyField->flags = css_bitflag_none;
            }

            outkeys = 1;
        }
        else {
            printf("Error: css key has too many chars: %s\n", begin);
            abort();
        }
    }

    return outkeys;
}


// 检查并设置索引
// 成功返回 0
static int CssKeyArrayCheckIndex(const CssString cssString, CssKeyArray cssKeys, int numKeys)
{
    struct CssKeyField* NotKey = cssKeys + numKeys;
    struct CssKeyField* start = cssKeys;
    struct CssKeyField* offkey = start;

    if (numKeys < 2 || !cssKeyTypeIsClass(offkey->type)) {
        return 1;
    }

    while (++offkey != NotKey) {
        if (!cssKeyTypeIsClass(offkey->type)) { // offkey meets {k:v}
            if (cssKeyTypeIsClass(start->type)) {
                while (start != offkey) {
                    start++->keyidx = (unsigned int)(offkey - cssKeys);
                }
                DEBUG_ASSERT(start == offkey)
            }
        }
        else { // offkey meets class
            if (!cssKeyTypeIsClass(start->type)) {
                start = offkey;
            }
        }
    }
    return 0;
}


CssKeyArray CssKeyArrayNew(int num)
{
    CssKeyArray keys = (CssKeyArray)malloc(sizeof(struct CssKeyField) * num);
    if (!keys) {
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


char* CssParseString(char* cssString, CssKeyArray outKeys, int* numKeys)
{
    int p, q, len;
    char tmpChar, * markStr;

    char* begin, * end, * start, * next;
    int keys = 0;

    char* css = cssString;
    int cssLen = (int)strnlen(cssString, CSS_STRING_MAXSIZE);

    if (cssLen == CSS_STRING_MAXSIZE) {
        printf("Error: cssString has too many chars\n");
        // 错误返回空, numKeys 保存错误码
        *numKeys = 1;
        return 0;
    }

    while (*css) {
        tmpChar = *css;
        if (tmpChar == 9 || tmpChar == 13 || tmpChar == 34 || tmpChar == 39) {
            // 用空格替代字符: [\t \r " ']
            *css = 32;  // space
        }
        else if (tmpChar == 10) {
            // 用 ; 替代换行符: [\n]
            *css = 59;
        }
        css++;
    }

    // 用空格替换注释: "/* ... */"
    struct small_regex* recomment = regex_compile("/\\*.*?\\*/");
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

            while (len-- > 0) {
                *start++ = 32;
            }
            css = next;
        }
    }
    regex_free(recomment);

    // 查找每个属性集: "{ key: value; ... }"
    struct small_regex* reclass = regex_compile("{.*?}");
    struct small_regex* rekey = regex_compile(":.*?;");
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
                if (cssKeyTypeIsClass(*begin)) {
                    keytype = (CssKeyType) *begin;
                    break;
                }
                begin++;
                p--;
            }

            if (p > 0) {
                // 如果发现选择器
                if (outKeys && keys < *numKeys) {
                    keys += setCssKeyField(cssString, &outKeys[keys], keytype, begin, p);
                }
                else {
                    keys += setCssKeyField(cssString, 0, keytype, begin, p);
                }
                CssCheckNumKeys(keys);

                markStr = start + len - 1;
                DEBUG_ASSERT(*markStr == '}')
                * markStr = ';';
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
                        keys += setCssKeyField(cssString, &outKeys[keys], css_type_key, start, q);
                    }
                    else {
                        keys += setCssKeyField(cssString, 0, css_type_key, start, q);
                    }
                    CssCheckNumKeys(keys);

                    // set value
                    if (outKeys && keys < *numKeys) {
                        keys += setCssKeyField(cssString, &outKeys[keys], css_type_value, begin, len);
                    }
                    else {
                        keys += setCssKeyField(cssString, 0, css_type_value, begin, len);
                    }
                    CssCheckNumKeys(keys);

                    start = end;
                }

                DEBUG_ASSERT(*markStr == ';')
                * markStr = '}';

                DEBUG_ASSERT(*next == '\0')
                * next = tmpChar;
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
        return cssString;
    }

    *numKeys = CssKeyArrayCheckIndex(cssString, outKeys, keys);
    if (*numKeys) {
        // 失败返回 NULL, 同时 *numKeys 设置错误码
        printf("Error: CssKeyArrayCheckIndex() error#%d\n", *numKeys);
        return 0;
    }

    // 成功返回指针, 同时保存 numKeys 保存 Keys 数目
    *numKeys = keys;
    return cssString;
}


CssString CssParseFile(FILE* cssFile, CssKeyArray outKeys, int* numKeys)
{
    rewind(cssFile);
    fseek(cssFile, 0, SEEK_END);
    int bsize = (int)ftell(cssFile);
    rewind(cssFile);

    if (bsize >= 0x03FFFF) {
        printf("Error: css file is too big\n");
        return 0;
    }

    char* cssString = (char*)malloc(bsize + sizeof(char));
    fread(cssString, sizeof(char), bsize, cssFile);
    cssString[bsize] = '\0';

    char* cssOut = CssParseString(cssString, outKeys, numKeys);
    if (!cssOut) {
        // 解析发生错误, 释放内存
        CssStringFree(cssString);
    }
    // 如果返回非空, 则用户使用完负责释放空间: CssStringFree(cssOut)
    return cssOut;
}


void CssKeyArrayPrint(const CssString cssString, const CssKeyArray cssKeys, int numKeys, FILE* fpout)
{
    struct CssKeyField* cssKey, *pair;
    char bitflags[4096];

    int nk = 0;

    while (nk < numKeys - 1) {
        cssKey = &cssKeys[nk++];

        if (cssKeyTypeIsClass(cssKey->type)) {
            int blen = cssKeyBitFlagToString(cssKey->flags, bitflags, sizeof(bitflags) - 1);

            int keyidx = cssKey->keyidx;

            fprintf(fpout, "%.*s %.*s{\n", (int)cssKey->length, &cssString[cssKey->offset], blen, bitflags);

            while (keyidx < numKeys) {
                if (cssKeyTypeIsClass(cssKeys[keyidx].type)) {
                    break;
                }
                pair = &cssKeys[keyidx++];

                if (pair->type == css_type_key) {
                    fprintf(fpout, "  %.*s:", (int)pair->length, &cssString[pair->offset]);
                }
                else {
                    DEBUG_ASSERT(pair->type == css_type_value)
                    fprintf(fpout, " %.*s;\n", (int)pair->length, &cssString[pair->offset]);
                }
            }

            fprintf(fpout, "}\n");
        }
    }
}
