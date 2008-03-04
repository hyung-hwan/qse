#
# You may override the value of MODE by specifying from the command-line
# make -f msw-dmc.mak MODE=debug
#

NAME = asecmn
MODE = release

CC = dmc
AR = lib

CFLAGS = -mn -I..\.. -DUNICODE -D_UNICODE 

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\mem.obj \
	$(TMP_DIR)\str.obj \
	$(TMP_DIR)\map.obj \
	$(TMP_DIR)\rex.obj \
	$(TMP_DIR)\misc.obj 

all: $(OUT_FILE_LIB) 

$(OUT_FILE_LIB): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_LIB)
	$(AR) -c $(OUT_FILE_LIB) $(OBJ_FILES_LIB)

$(TMP_DIR)\mem.obj: mem.c
	$(CC) $(CFLAGS) -o$@ -c mem.c

$(TMP_DIR)\str.obj: str.c
	$(CC) $(CFLAGS) -o$@ -c str.c

$(TMP_DIR)\map.obj: map.c
	$(CC) $(CFLAGS) -o$@ -c map.c

$(TMP_DIR)\rex.obj: rex.c
	$(CC) $(CFLAGS) -o$@ -c rex.c

$(TMP_DIR)\misc.obj: misc.c
	$(CC) $(CFLAGS) -o$@ -c misc.c

$(OUT_DIR):
	md $(OUT_DIR)

$(TMP_DIR):
	md $(TMP_DIR)

clean:
	del $(OUT_FILE_LIB) $(OBJ_FILES_LIB) 

