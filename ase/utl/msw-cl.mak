NAME = aseutl
MODE = release

CC = cl
LD = link
AR = link 

CFLAGS = /nologo /O2 /MT /W3 -I..\..

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\main.obj \
	$(TMP_DIR)\ctype.obj \
	$(TMP_DIR)\stdio.obj

all: lib

lib: $(OUT_FILE_LIB) 

$(OUT_FILE_LIB): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_LIB)
	$(AR) /lib @<<
/nologo /out:$(OUT_FILE_LIB) $(OBJ_FILES_LIB)
<<

$(TMP_DIR)\main.obj: main.c
	$(CC) $(CFLAGS) /Fo$@ /c main.c

$(TMP_DIR)\ctype.obj: ctype.c
	$(CC) $(CFLAGS) /Fo$@ /c ctype.c

$(TMP_DIR)\stdio.obj: stdio.c
	$(CC) $(CFLAGS) /Fo$@ /c stdio.c

$(OUT_DIR):
	md $(OUT_DIR)

$(TMP_DIR):
	md $(TMP_DIR)

clean:
	del $(OUT_FILE_LIB) 
	del $(OBJ_FILES_LIB) 

