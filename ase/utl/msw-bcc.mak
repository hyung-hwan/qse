NAME = aseutl

!ifndef MODE
MODE = release
!endif

CC = bcc32 
LD = ilink32
AR = tlib 

CFLAGS = -O2 -WM -WU -RT- -w -q -I..\..

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\main.obj \
	$(TMP_DIR)\ctype.obj \
	$(TMP_DIR)\stdio.obj 

all: lib

lib: $(TMP_DIR) $(OUT_DIR) $(OUT_FILE_LIB) 

$(OUT_FILE_LIB): $(OBJ_FILES_LIB)
	$(AR) $(OUT_FILE_LIB) @&&!
+-$(**: = &^
+-)
!

$(TMP_DIR)\main.obj: main.c
	$(CC) $(CFLAGS) -o$@ -c main.c

$(TMP_DIR)\ctype.obj: ctype.c
	$(CC) $(CFLAGS) -o$@ -c ctype.c

$(TMP_DIR)\stdio.obj: stdio.c
	$(CC) $(CFLAGS) -o$@ -c stdio.c

$(OUT_DIR):
	-md $(OUT_DIR) 

$(TMP_DIR):
	-md $(TMP_DIR)

clean:
	-del $(OUT_FILE_LIB) 
	-del $(OBJ_FILES_LIB) 

