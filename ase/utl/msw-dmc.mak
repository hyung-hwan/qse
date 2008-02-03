#
# You may override the value of MODE by specifying from the command-line
# make -f msw-dmc.mak MODE=debug
#

NAME = aseutl
MODE = release

CC = dmc
AR = lib

CFLAGS = -mn -I..\.. -DUNICODE -D_UNICODE 

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\main.obj \
	$(TMP_DIR)\ctype.obj \
	$(TMP_DIR)\stdio.obj \
	$(TMP_DIR)\http.obj

all: $(OUT_FILE_LIB) 

$(OUT_FILE_LIB): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_LIB)
	$(AR) -c $(OUT_FILE_LIB) $(OBJ_FILES_LIB)

$(TMP_DIR)\main.obj: main.c
	$(CC) $(CFLAGS) -o$@ -c main.c

$(TMP_DIR)\ctype.obj: ctype.c
	$(CC) $(CFLAGS) -o$@ -c ctype.c

$(TMP_DIR)\stdio.obj: stdio.c
	$(CC) $(CFLAGS) -o$@ -c stdio.c

$(TMP_DIR)\http.obj: http.c
	$(CC) $(CFLAGS) -o$@ -c http.c

$(OUT_DIR):
	md $(OUT_DIR)

$(TMP_DIR):
	md $(TMP_DIR)

clean:
	del $(OUT_FILE_LIB) $(OBJ_FILES_LIB) 

