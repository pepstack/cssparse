/**
 * @file mycssparse.c
 *
 * @brief 解析 CSS 文件
 *
 * @copyright Copyright (c) 2024, mapaware.top
 * @author 350137278@qq.com
 *
 * @since 2024-10-08 23:49:15
 * @date 2024-10-10 11:15:05
 * @version 0.1.5
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


void demo_cssparse_string(char *cssString, FILE * outcssfile)
{
    printf("parse css string:\n--------\n%s\n--------\n", cssString);

    int numKeys = 0;
    CssKeyArray cssOutKeys = 0;

    // 测试空间以分配内存
    numKeys = CssParseString(cssString, 0);
    if (numKeys < 0) {
        cssOutKeys = CssKeyArrayNew(-numKeys);
        if (CssParseString(cssString, cssOutKeys) > 0) {
            // 使用 cssOutKeys

            CssKeyArrayPrint(cssString, cssOutKeys, (outcssfile ? outcssfile : stdout));

            // TODO:
            // ...
        }
        CssKeyArrayFree(cssOutKeys);
    }

    free(cssString);
}


void demo_cssparse_file(const char* csspathfile, FILE* cssFileOut)
{
    // 必须一次读全部输入文件内容
    FILE* cssFileIn = fopen(csspathfile, "r");
    if (!cssFileIn) {
        printf("Error: open file failed: %s\n", csspathfile);
        exit(1);
    }
    rewind(cssFileIn);
    fseek(cssFileIn, 0, SEEK_END);
    int bsize = (int)ftell(cssFileIn);
    if (bsize >= CSS_STRING_BSIZE_MAX_1048576) {
        printf("Error: css file is too big.\n");
        fclose(cssFileIn);
        exit(1);
    }

    rewind(cssFileIn);
    char* cssString = (char*) malloc(bsize + sizeof(char));
    fread(cssString, sizeof(char), bsize, cssFileIn);
    cssString[bsize] = '\0';
    fclose(cssFileIn);

    demo_cssparse_string(cssString, cssFileOut);
}


int main(int argc, char * argv[])
{
    if (argc == 1) {
        print_usage(argv[0]);
        return 1;
    }

    FILE* cssFileOut = 0;

    if (argc == 3 && strstr(argv[2], "file://") == argv[2]) {
        const char* outcssfile = argv[2] + 7;

        if (file_exists(outcssfile)) {
            printf("Error: output css file existed: %s\n", outcssfile);
            exit(1);
        }
        cssFileOut = fopen(outcssfile, "w+");
        if (!cssFileOut) {
            printf("Error: open file failed: %s\n", outcssfile);
            exit(1);
        }
    }

    if (strstr(argv[1], "file://") == argv[1]) {
        demo_cssparse_file(argv[1] + 7, cssFileOut);
    } else {
        int csslen = (int)strnlen(argv[1], 0xfff);
        if (csslen == 0xfff) {
            printf("Error: Input css string is too long.\n");
            exit(1);
        }
        char* cssString = (char*)malloc((csslen + 1) * sizeof(char));
        if (!cssString) {
            printf("Error: Out of memory.\n");
            abort();
        }
        memcpy(cssString, argv[1], (csslen + 1) * sizeof(char));

        demo_cssparse_string(cssString, cssFileOut);

        free(cssString);
    }

    if (cssFileOut) {
        fclose(cssFileOut);
    }
    return 0;
}
