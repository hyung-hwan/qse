NAME = aseawk
MODE = release

CC = cl
CXX = cl
LD = link

CFLAGS = /nologo /W3 -I..\..\.. 
CXXFLAGS = /nologo /W3 -I..\..\..

LDFLAGS = /libpath:..\..\$(MODE)\lib
LIBS = asecmn.lib aseawk.lib aseutl.lib kernel32.lib user32.lib
LIBS_CXX = $(LIBS) aseawk++.lib

!IF "$(MODE)" == "debug"
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG /MTd
CXXFLAGS = $(CXXFLAGS) -D_DEBUG -DDEBUG /MTd
!ELSEIF "$(MODE)" == "release"
CFLAGS = $(CFLAGS) -DNDEBUG /MT /O2
CXXFLAGS = $(CXXFLAGS) -DNDEBUG /MT /O2
!ELSE
CFLAGS = $(CFLAGS) /MT
CXXFLAGS = $(CXXFLAGS) /MT
!ENDIF

!if !defined(CPU) || "$(CPU)" == ""
CPU = $(PROCESSOR_ARCHITECTURE)
!endif 

!if "$(CPU)" == ""
CPU = i386
!endif

!if "$(CPU)" == "IA64" || "$(CPU)" == "AMD64"
# comment out the following line if you encounter this link error.
#    LINK : fatal error LNK1181: cannot open input file 'bufferoverflowu.lib'
LIBS = $(LIBS) bufferoverflowu.lib
!endif

OUT_DIR = ..\..\$(MODE)\bin
OUT_FILE_BIN = $(OUT_DIR)\$(NAME).exe
OUT_FILE_BIN_CXX = $(OUT_DIR)\$(NAME)++.exe

TMP_DIR = $(MODE)
TMP_DIR_CXX = $(TMP_DIR)\cxx

OBJ_FILES_BIN = $(TMP_DIR)\awk.obj
OBJ_FILES_BIN_CXX = $(TMP_DIR_CXX)\Awk.obj

all: bin

bin: $(OUT_FILE_BIN) $(OUT_FILE_BIN_CXX)

$(OUT_FILE_BIN): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_BIN)
	$(LD) /nologo /out:$@ $(LDFLAGS) $(LIBS) $(OBJ_FILES_BIN)

$(OUT_FILE_BIN_CXX): $(TMP_DIR_CXX) $(OUT_FILE_BIN) $(OBJ_FILES_BIN_CXX)
	$(LD) /nologo /out:$@ $(LDFLAGS) $(LIBS_CXX) $(OBJ_FILES_BIN_CXX)

$(TMP_DIR)\awk.obj: awk.c
	$(CC) $(CFLAGS) /Fo$@ /c awk.c

$(TMP_DIR_CXX)\Awk.obj: Awk.cpp
	$(CC) $(CXXFLAGS) /Fo$@ /c Awk.cpp

$(OUT_DIR):
	-md $(OUT_DIR)

$(TMP_DIR):
	-md $(TMP_DIR)

$(TMP_DIR_CXX): $(TMP_DIR)
	-md $(TMP_DIR_CXX)

clean:
	-del $(OUT_FILE_BIN) 
	-del $(OUT_FILE_BIN_CXX) 
	-del $(OBJ_FILES_BIN) 
	-del $(OBJ_FILES_BIN_CXX) 

