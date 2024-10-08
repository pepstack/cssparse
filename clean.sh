#!/bin/bash
#
# @file clean.sh
#   remove all mediate files
#
# @author mapaware@hotmail.com
# @copyright © 2024-2030 mapaware.top All Rights Reserved.
# @version 1.2.11
#
# @since 2024-10-08 23:42:33
# @date 2024-10-09 01:50:48
#
#######################################################################
_name_=$(basename "$0")
_cdir_=$(cd "$(dirname "$0")" && pwd)
_file_=""${_cdir_}"/"${_name_}""

# Treat unset variables as an error
set -o nounset

# Treat any error as exit
set -o errexit

APPNAME="$1"

# 确保不会误删除目录!!
if [ -d "$APPNAME" ]; then
    echo "$_name_: CAN NOT be a path: $APPNAME"
    exit
fi

#######################################################################

echo "$_name_: remove all mediate files under: ${_cdir_}"

cd ${_cdir_} && rm -f *.o "./${APPNAME}" "./${APPNAME}.exe"

cd ${_cdir_} && rm -rf "./temp" "./projects/msvc/${APPNAME}/vcbuild" "./projects/msvc/${APPNAME}/.vs"

echo "$_name_: done"