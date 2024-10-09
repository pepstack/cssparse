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
 * @version 0.0.10
 * @since     2024-10-07
 * @date 2024-10-10 02:25:58
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "cssparse.h"
#include "smallregex.h"


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


#define CssKeyArraySetSize(cssKeys)  (int)(((struct CssKeyField*)(cssKeys - 1))->numKeys)


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
    union {
        struct {
            int32_t  SizeKeys;
            int32_t  UsedKeys;
        };
        struct {
            unsigned int flags : 16;  // 16 个标记位
            unsigned int type : 8;    // 类型
            unsigned int length : 8;  // 名称长度
            unsigned int offset : 20;  // <= 0xFFFFF
            unsigned int keyidx : 12;  // 如果当前是 class, 这个值为定义 class 的 {key:value ...} 的索引位置
        };
    };
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


static int cssParseClassFlags(const char* start, int len, int offsets[], int lengths[], int classflags[], int nsize)
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
// 成功返回 UsedKeys, 失败返回 0
static int CssKeyArrayBuild(const char* cssString, CssKeyArray cssKeys, int numKeys)
{
    struct CssKeyField* NotKey = cssKeys + numKeys;
    struct CssKeyField* start = cssKeys;
    struct CssKeyField* offkey = start;

    if (numKeys < 2 || ! cssKeyTypeIsClass(offkey->type)) {
        return 0;
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

    ((struct CssKeyField*)(cssKeys - 1))->UsedKeys = numKeys;
    return numKeys;
}


CssKeyArray CssKeyArrayNew(int num)
{
    if (num > CSS_KEYINDEX_MAX) {
        printf("Error: too many keys.\n");
        abort();
    }
    CssKeyArray keys = (CssKeyArray) calloc(num + 1, sizeof(struct CssKeyField));
    if (! keys) {
        printf("Error: out of memory\n");
        abort();
    }

    keys++->SizeKeys = num;
    return keys;
}


void CssKeyArrayFree(CssKeyArray keys)
{
    if (keys--) {
        free(keys);
    }
}


int CssKeyArrayGetSize(const CssKeyArray cssKeys)
{
    if (cssKeys) {
        return (int)((struct CssKeyField*)(cssKeys - 1))->SizeKeys;
    }
    return 0;
}


int CssKeyArrayGetUsed(const CssKeyArray cssKeys)
{
    if (cssKeys) {
        return (int)((struct CssKeyField*)(cssKeys - 1))->UsedKeys;
    }
    return 0;
}


int CssParseString(char* cssString, CssKeyArray outKeys)
{
    int p, q, len;
    char tmpChar, * markStr;

    char* begin, * end, * start, * next;
    int keys = 0;

    char* css = cssString;
    int cssLen = (int)strnlen(cssString, CSS_STRING_MAXSIZE);

    int SizeKeys = CssKeyArrayGetSize(outKeys);

    if (cssLen == CSS_STRING_MAXSIZE) {
        printf("Error: cssString has too many chars.\n");

        // 返回 0 表示错误
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
    regex_free(recomment);

    // 查找每个属性集: "{ key: value; ... }"
    struct small_regex* reclass = regex_compile("{.*?}");
    struct small_regex* rekey = regex_compile(":.*?;");

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
            keys += setCssKeyField(cssString, ((outKeys && keys < SizeKeys) ? &outKeys[keys] : 0),
                        keytype, begin, p);
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
                keys += setCssKeyField(cssString, ((outKeys && keys < SizeKeys) ? &outKeys[keys] : 0),
                            css_type_key, start, q);
                CssCheckNumKeys(keys);

                // set value
                keys += setCssKeyField(cssString, ((outKeys && keys < SizeKeys) ? &outKeys[keys] : 0),
                            css_type_value, begin, len);
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
    regex_free(rekey);
    regex_free(reclass);

    // 用户必须判断返回的 keys > 0
    if (keys > SizeKeys) {
        // 负值表示输入的空间不够, 其绝对值为需要的空间大小
        return -keys;
    }

    if (! CssKeyArrayBuild(cssString, outKeys, keys)) {
        // 失败返回 0
        printf("Error: CssKeyArrayBuild() failed.\n");
        return 0;
    }

    // 成功返回保存 Keys 数目
    return CssKeyArrayGetUsed(outKeys);
}


void CssKeyArrayPrint(const char *cssString, const CssKeyArray cssKeys, FILE* fpout)
{
    struct CssKeyField* cssKey, *pair;
    char bitflags[4096];

    int nk = 0;

    int numKeys = CssKeyArrayGetUsed(cssKeys);

    while (nk < numKeys - 1) {
        cssKey = CssKeyArrayGetNode(cssKeys, nk++);

        if (cssKeyTypeIsClass(CssKeyGetType(cssKey))) {
            int blen = CssKeyFlagToString(CssKeyGetFlag(cssKey), bitflags, sizeof(bitflags) - 1);

            int keyidx = cssKey->keyidx;

            int offset = 0;
            int length = CssKeyOffsetLength(cssKey, &offset);

            fprintf(fpout, "%.*s %.*s{\n", length, &cssString[offset], blen, bitflags);

            while (keyidx < numKeys) {
                pair = CssKeyArrayGetNode(cssKeys, keyidx++);
                if (CssKeyTypeIsClass(pair)) {
                    break;
                }

                length = CssKeyOffsetLength(pair, &offset);
                if (CssKeyGetType(pair) == css_type_key) {
                    fprintf(fpout, "  %.*s:", length, &cssString[offset]);
                }
                else {
                    DEBUG_ASSERT(CssKeyGetType(pair) == css_type_value)
                    fprintf(fpout, " %.*s;\n", length, &cssString[offset]);
                }
            }

            fprintf(fpout, "}\n");
        }
    }
}


const CssKeyArrayNode CssKeyArrayGetNode(const CssKeyArray cssKeys, int index)
{
    int numKeys = CssKeyArrayGetUsed(cssKeys);
    if (index < numKeys) {
        return (CssKeyArrayNode)(cssKeys + index);
    }
    return 0;
}


CssKeyType CssKeyGetType(const CssKeyArrayNode cssKey)
{
    return cssKey->type;
}


int CssKeyGetFlag(const CssKeyArrayNode cssKey)
{
    return (int)(cssKey->flags);
}


int CssKeyOffsetLength(const CssKeyArrayNode cssKeyNode, int *bOffset)
{
    *bOffset = (int)cssKeyNode->offset;
    return (int)cssKeyNode->length;
}


int CssKeyTypeIsClass(const CssKeyArrayNode cssKeyNode)
{
    return cssKeyTypeIsClass(cssKeyNode->type);
}


int CssKeyFlagToString(int keyflag, char* outbuf, size_t buflen)
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
