# @file Makefile
#
#   源代码文件修改之后，运行这个脚本会自动更新代码的版本和时间
#
# @author mapaware@hotmail.com
# @copyright © 2024-2030 mapaware.top All Rights Reserved.
# @version 1.0.7
#
# @since 2024-10-08 19:24:50
# @date 2024-10-09 01:33:57
#
################################################################
# compiler
CC=gcc

# compile directives
CFLAGS += -std=gnu11 -D_GNU_SOURCE -fPIC -Wall -Wno-unused-function -Wno-unused-variable

# load libs: -lpthread = libpthread.so
LDFLAGS += -lm -lpthread


# 主程序名
APPNAME := mycssparse

PREFIX = .

SOURCEDIR = $(PREFIX)/source

INCDIRS += -I$(SOURCEDIR) -I$(SOURCEDIR)/common

ALLCDIRS += $(SOURCEDIR) $(SOURCEDIR)/common

CSRCS := $(foreach cdir, $(ALLCDIRS), $(wildcard $(cdir)/*.c))

COBJS = $(patsubst %.c, %.o, $(notdir $(CSRCS)))

################################################################
.PHONY: all test clean revise-source


all: $(APPNAME)

define COBJS_template =
$(basename $(notdir $(1))).o: $(1)
	$(CC) $(CFLAGS) -c $(1) $(INCDIRS) -o $(basename $(notdir $(1))).o
endef

$(foreach src,$(CSRCS),$(eval $(call COBJS_template,$(src))))


$(APPNAME): $(COBJS)
	@echo "Build $(BUILD) $(APPNAME)"
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Done."


clean:
	@$(PREFIX)/clean.sh $(APPNAME)


revise-source:
	@/usr/bin/find . -type f -mtime -30 \( -name '*.h' -o -name '*.c' \) | xargs -I {} sh -c "sh revise-source.sh {}"
	@/usr/bin/find . -type f -mtime -30 \( -name '*.hxx' -o -name '*.cxx' \) | xargs -I {} sh -c "sh revise-source.sh {}"
	@/usr/bin/find . -type f -mtime -30 \( -name '*.hpp' -o -name '*.cpp' \) | xargs -I {} sh -c "sh revise-source.sh {}"
	@/usr/bin/find . -type f -mtime -30 \( -name '*.java' -o -name '*.py' \) | xargs -I {} sh -c "sh revise-source.sh {}"
	@/usr/bin/find . -type f -mtime -30 -name '*.sh' | xargs -I {} sh -c "sh revise-source.sh {}"
	@/usr/bin/find . -type f -mtime -30 -name 'Makefile' | xargs -I {} sh -c "sh revise-source.sh {}"
	@echo "(Ok) revise source files done."

test: all
	$(PREFIX)/$(APPNAME).exe ".polygon { border: 3px solid #ff00ff; fill: 0.5 solid #00f0f0 }"
