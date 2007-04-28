NAME = asecmn
MODE = release

CC = cl
LD = link
AR = link 

CFLAGS = /nologo /O2 /MT /W3 -I..\..

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\mem.obj \
	$(TMP_DIR)\str.obj \
	$(TMP_DIR)\misc.obj

all: lib

lib: $(OUT_FILE_LIB) 

$(OUT_FILE_LIB): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_LIB)
	$(AR) /lib @<<
/nologo /out:$(OUT_FILE_LIB) $(OBJ_FILES_LIB)
<<

$(TMP_DIR)\mem.obj: mem.c
	$(CC) $(CFLAGS) /Fo$@ /c mem.c

$(TMP_DIR)\str.obj: str.c
	$(CC) $(CFLAGS) /Fo$@ /c str.c

$(TMP_DIR)\misc.obj: misc.c
	$(CC) $(CFLAGS) /Fo$@ /c misc.c

$(OUT_DIR):
	md $(OUT_DIR)

$(TMP_DIR):
	md $(TMP_DIR)

clean:
	del $(OUT_FILE_LIB) 
	del $(OBJ_FILES_LIB) 

