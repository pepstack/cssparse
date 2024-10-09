/**
 * @file mycssparse.c
 *
 * @brief 解析 CSS 文件
 *
 * @copyright Copyright (c) 2024, mapaware.top
 * @author 350137278@qq.com
 *
 * @since 2024-10-08 23:49:15
 * @date 2024-10-09 18:46:20
 * @version 0.1.2
 *
 * @note
 *  Compile:
 *     $ make
 *
 *  Usage:
 *    1) 解析输入文件, 输出到输出文件
 *      $ mycssparse file:///path/to/input1.css file:///path/to/output1.css
 *
 *    2) 解析字符串输入, 输出结果到终端
 *      $ mycssparse ".polygon { border: 3px solid #ff00ff; fill: 0.5 solid #00f0f0 }"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <common/cssparse.h>


void print_usage(const char *appfile)
{
    char path[256] = {0};

    snprintf(path, sizeof(path), "%s", appfile);

    char *p = path;
    while(*p) {
        if (*p == '\\') {
            *p = '/';
        }
        p++;
    }
    char *name = strrchr(path, '/');
    *name++ = '\0';

    printf("%s\n", name);
    printf("    Parse input css strinf or file and output parsed css.\n");
    printf("  Usage:\n");
    printf("    $ %s input-css-file <output-css-file>\n", name);
    printf("    $ %s input-css-string <output-css-file>\n", name);
    printf("\n");
}


void cssparse_file(const char *incssfile, const char *outcssfile)
{
    printf("parse css file: %s\n", incssfile);

    CssString cssString = 0;
    int numKeys = 0;
    CssKeyArray cssOutKeys = 0;

    FILE * fp = fopen(incssfile, "r");
    if (! fp) {
        printf("Error: Bad input css file: %s\n", incssfile);
        return;
    }

    // 测试空间以分配内存
    cssString = CssParseFile(fp, 0, &numKeys);
    fclose(fp);

    if (cssString && numKeys < 0) {
        numKeys = -numKeys;
        cssOutKeys = CssKeyArrayNew(numKeys + 200);
        numKeys += 200; //??

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


void cssparse_string(const char *incssstring, const char *outcssfile)
{
    printf("parse css string:\n--------\n%s\n--------\n", incssstring);

    int csslen = (int) strnlen(incssstring, 0xffff);
    if (csslen == 0xffff) {
        printf("Error: Input css string is too long.\n");
        return;
    }

    char *cssString = (char *) malloc((csslen + 1) * sizeof(char));
    if (! cssString) {
        printf("Error: Out of memory.\n");
        abort();
    }

    memcpy(cssString, incssstring, (csslen + 1) * sizeof(char));

    int numKeys = 0;
    CssKeyArray cssOutKeys = 0;

    // 测试空间以分配内存
    if (CssParseString(cssString, 0, &numKeys) && numKeys < 0) {
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

    free(cssString);
}


int main(int argc, char * argv[])
{
    if (argc == 1) {
        print_usage(argv[0]);
        return 1;
    }

    if (strstr(argv[1], "file://") == argv[1]) {
        cssparse_file(argv[1] + 7, (argc > 2? argv[2] : 0));
    } else {
        cssparse_string(argv[1], (argc > 2? argv[2] : 0));
    }

    return 0;
}
