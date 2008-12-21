NAME = asecmn

!ifndef MODE
MODE = release
!endif

CC = bcc32 
LD = ilink32
AR = tlib 

CFLAGS = -WM -WU -RT- -w -q -I..\..

!IF "$(MODE)" == "debug"
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG 
!ELSEIF "$(MODE)" == "release"
CFLAGS = $(CFLAGS) -DNDEBUG -O2
!ELSE
CFLAGS = $(CFLAGS)
!ENDIF

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib

TMP_DIR = $(MODE)

OBJ_FILES_LIB = \
	$(TMP_DIR)\mem.obj \
	$(TMP_DIR)\str_bas.obj \
	$(TMP_DIR)\str_cnv.obj \
	$(TMP_DIR)\str_dyn.obj \
	$(TMP_DIR)\map.obj \
	$(TMP_DIR)\rex.obj \
	$(TMP_DIR)\misc.obj 

all: lib

lib: $(TMP_DIR) $(OUT_DIR) $(OUT_FILE_LIB) 

$(OUT_FILE_LIB): $(OBJ_FILES_LIB)
	$(AR) $(OUT_FILE_LIB) @&&!
+-$(**: = &^
+-)
!

$(TMP_DIR)\mem.obj: mem.c
	$(CC) $(CFLAGS) -o$@ -c mem.c

$(TMP_DIR)\str_bas.obj: str_bas.c
	$(CC) $(CFLAGS) -o$@ -c str_bas.c

$(TMP_DIR)\str_cnv.obj: str_cnv.c
	$(CC) $(CFLAGS) -o$@ -c str_cnv.c

$(TMP_DIR)\str_dyn.obj: str_dyn.c
	$(CC) $(CFLAGS) -o$@ -c str_dyn.c

$(TMP_DIR)\map.obj: map.c
	$(CC) $(CFLAGS) -o$@ -c map.c

$(TMP_DIR)\rex.obj: rex.c
	$(CC) $(CFLAGS) -o$@ -c rex.c

$(TMP_DIR)\misc.obj: misc.c
	$(CC) $(CFLAGS) -o$@ -c misc.c

$(OUT_DIR):
	-md $(OUT_DIR) 

$(TMP_DIR):
	-md $(TMP_DIR)

clean:
	-del $(OUT_FILE_LIB) 
	-del $(OBJ_FILES_LIB) 

