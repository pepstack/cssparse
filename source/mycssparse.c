/**
 * @file mycssparse.c
 *
 * @brief 解析 CSS 文件
 *
 * @copyright Copyright (c) 2024, mapaware.top
 * @author 350137278@qq.com
 *
 * @since 2024-10-08 23:49:15
 * @date 2024-10-09 23:30:52
 * @version 0.1.3
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
#include <sys/stat.h>

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


int file_exists(const char* filename)
{
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}


void cssparse_file(const char *incssfile, FILE * outcssfile)
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
            CssKeyArrayPrint(cssString, cssOutKeys, numKeys, (outcssfile? outcssfile : stdout));
            // TODO:
            // ...
        }

        CssKeyArrayFree(cssOutKeys);
    }
    CssStringFree(cssString);
}


void cssparse_string(const char *incssstring, FILE * outcssfile)
{
    printf("parse css string:\n--------\n%s\n--------\n", incssstring);

    int csslen = (int) strnlen(incssstring, 0xffff);
    if (csslen == 0xffff) {
        printf("Error: Input css string is too long.\n");
        exit(1);
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

            CssKeyArrayPrint(cssString, cssOutKeys, numKeys, (outcssfile ? outcssfile : stdout));

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

    FILE* fpoutcss = 0;

    if (argc == 3 && strstr(argv[2], "file://") == argv[2]) {
        const char* outcssfile = argv[2] + 7;

        if (file_exists(outcssfile)) {
            printf("Error: output css file existed: %s\n", outcssfile);
            exit(1);
        }
        fpoutcss = fopen(outcssfile, "w+");
        if (!fpoutcss) {
            printf("Error: open file failed: %s\n", outcssfile);
            exit(1);
        }
    }

    if (strstr(argv[1], "file://") == argv[1]) {
        cssparse_file(argv[1] + 7, fpoutcss);
    } else {
        cssparse_string(argv[1], fpoutcss);
    }

    if (fpoutcss) {
        fclose(fpoutcss);
    }
    return 0;
}
