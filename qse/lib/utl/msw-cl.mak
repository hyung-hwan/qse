NAME = aseutl
MODE = release

CC = cl
LD = link
AR = link 

CFLAGS = /nologo /W3 -I..\..

!IF "$(MODE)" == "debug"
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG /MTd /Zi
!ELSEIF "$(MODE)" == "release"
CFLAGS = $(CFLAGS) -DNDEBUG /MT /O2
!ELSE
CFLAGS = $(CFLAGS) /MT
!ENDIF

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\main.obj \
	$(TMP_DIR)\ctype.obj \
	$(TMP_DIR)\stdio.obj \
	$(TMP_DIR)\http.obj

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

$(TMP_DIR)\http.obj: http.c
	$(CC) $(CFLAGS) /Fo$@ /c http.c

$(OUT_DIR):
	-md $(OUT_DIR)

$(TMP_DIR):
	-md $(TMP_DIR)

clean:
	-del $(OUT_FILE_LIB) 
	-del $(OBJ_FILES_LIB) 

