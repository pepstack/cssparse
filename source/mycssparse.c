/**
 * @file mycssparse.c
 *
 * @brief 解析 CSS 文件
 *
 * @copyright Copyright (c) 2024, mapaware.top
 * @author 350137278@qq.com
 *
 * @since 2024-10-08 23:49:15
 * @date 2024-10-15 02:14:01
 * @version 0.1.6
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


void demo_cssparse_string(char* cssStr, FILE* outcssfile)
{
    printf("parse css string:\n--------\n%s\n--------\n", cssStr);

    CssString cssString = CssStringNew(cssStr, strlen(cssStr));
    CssKeyArray keys = CssStringParse(cssString);
    if (keys) {
        CssKeyArrayPrint(keys, outcssfile);
        CssKeyArrayFree(keys);
    }
    else {
        CssStringFree(cssString);
    }
}


void demo_cssparse_file(const char *csspathfile, FILE *cssFileOut)
{
    FILE* cssfile = fopen(csspathfile, "r");
    if (cssfile) {
        CssString cssString = CssStringNewFromFile(cssfile);
        fclose(cssfile);

        if (!cssString) {
            printf("Error: CssStringNewFromFile() failed. cssfile=%s\n", csspathfile);
            exit(1);
        }

        CssKeyArray keys = CssStringParse(cssString);
        if (!keys) {
            printf("Error: CssStringParse() failed\n");
            CssStringFree(cssString);
            exit(1);
        }

        CssKeyArrayPrint(keys, stdout);

        CssKeyArrayFree(keys);
    }
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
        demo_cssparse_string(argv[1], cssFileOut);
    }

    if (cssFileOut) {
        fclose(cssFileOut);
    }
    return 0;
}
