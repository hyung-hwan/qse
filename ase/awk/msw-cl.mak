NAME = aseawk
MODE = release

JNI_INC = \
	-I"$(JAVA_HOME)\include" \
	-I"$(JAVA_HOME)\include\win32" 

CC = cl
CXX = cl
LD = link
AR = link 
JAVAC = javac
JAR = jar

CFLAGS = /nologo /W3 -I..\..
CXXFLAGS = /nologo /W3 -I..\..
JAVACFLAGS = -classpath ..\.. -Xlint:unchecked

#LDFLAGS = /subsystem:console
LDFLAGS = /subsystem:windows

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

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib
OUT_FILE_JNI = $(OUT_DIR)\$(NAME)_jni.dll
OUT_FILE_LIB_CXX = $(OUT_DIR)\$(NAME)++.lib
OUT_FILE_JAR = $(OUT_DIR)\$(NAME).jar

TMP_DIR = $(MODE)
TMP_DIR_CXX = $(TMP_DIR)\cxx

OBJ_FILES_LIB = \
	$(TMP_DIR)\awk.obj \
	$(TMP_DIR)\err.obj \
	$(TMP_DIR)\tree.obj \
	$(TMP_DIR)\tab.obj \
	$(TMP_DIR)\map.obj \
	$(TMP_DIR)\parse.obj \
	$(TMP_DIR)\run.obj \
	$(TMP_DIR)\rec.obj \
	$(TMP_DIR)\val.obj \
	$(TMP_DIR)\func.obj \
	$(TMP_DIR)\misc.obj \
	$(TMP_DIR)\extio.obj \
	$(TMP_DIR)\rex.obj

OBJ_FILES_JNI = $(TMP_DIR)\jni.obj 

OBJ_FILES_LIB_CXX = \
	$(TMP_DIR)\cxx\Awk.obj \
	$(TMP_DIR)\cxx\StdAwk.obj

OBJ_FILES_JAR = \
	$(TMP_DIR)\ase\awk\Awk.class \
	$(TMP_DIR)\ase\awk\StdAwk.class \
	$(TMP_DIR)\ase\awk\Context.class \
	$(TMP_DIR)\ase\awk\Extio.class \
	$(TMP_DIR)\ase\awk\IO.class \
	$(TMP_DIR)\ase\awk\Console.class \
	$(TMP_DIR)\ase\awk\File.class \
	$(TMP_DIR)\ase\awk\Pipe.class \
	$(TMP_DIR)\ase\awk\Exception.class

all: lib

lib: $(OUT_FILE_LIB) $(OUT_FILE_LIB_CXX)

jnidll: $(OUT_FILE_JNI)

jar: $(OUT_FILE_JAR)

$(OUT_FILE_LIB): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_LIB)
	$(AR) /lib @<<
/nologo /out:$(OUT_FILE_LIB) $(OBJ_FILES_LIB)
<<

$(OUT_FILE_LIB_CXX): $(TMP_DIR_CXX) $(OUT_FILE_LIB) $(OBJ_FILES_LIB_CXX)
	$(AR) /lib @<<
/nologo /out:$(OUT_FILE_LIB_CXX) $(OBJ_FILES_LIB_CXX)
<<

$(OUT_FILE_JNI): $(OUT_FILE_LIB) $(OBJ_FILES_JNI)
	$(LD) /dll /def:jni.def $(LDFLAGS) /release @<<
/nologo /out:$(OUT_FILE_JNI) $(OBJ_FILES_JNI) /libpath:../$(MODE)/lib /implib:tmp.lib user32.lib $(OUT_FILE_LIB) asecmn.lib aseutl.lib
<<
	del tmp.lib tmp.exp


$(OUT_FILE_JAR): $(OBJ_FILES_JAR)
	$(JAR) -Mcvf $(OUT_FILE_JAR) -C $(TMP_DIR) ase

$(TMP_DIR)\awk.obj: awk.c
	$(CC) $(CFLAGS) /Fo$@ /c awk.c

$(TMP_DIR)\err.obj: err.c
	$(CC) $(CFLAGS) /Fo$@ /c err.c

$(TMP_DIR)\tree.obj: tree.c
	$(CC) $(CFLAGS) /Fo$@ /c tree.c

$(TMP_DIR)\tab.obj: tab.c
	$(CC) $(CFLAGS) /Fo$@ /c tab.c

$(TMP_DIR)\map.obj: map.c
	$(CC) $(CFLAGS) /Fo$@ /c map.c

$(TMP_DIR)\parse.obj: parse.c
	$(CC) $(CFLAGS) /Fo$@ /c parse.c

$(TMP_DIR)\run.obj: run.c
	$(CC) $(CFLAGS) /Fo$@ /c run.c

$(TMP_DIR)\rec.obj: rec.c
	$(CC) $(CFLAGS) /Fo$@ /c rec.c

$(TMP_DIR)\val.obj: val.c
	$(CC) $(CFLAGS) /Fo$@ /c val.c

$(TMP_DIR)\func.obj: func.c
	$(CC) $(CFLAGS) /Fo$@ /c func.c

$(TMP_DIR)\misc.obj: misc.c
	$(CC) $(CFLAGS) /Fo$@ /c misc.c

$(TMP_DIR)\extio.obj: extio.c
	$(CC) $(CFLAGS) /Fo$@ /c extio.c

$(TMP_DIR)\rex.obj: rex.c
	$(CC) $(CFLAGS) /Fo$@ /c rex.c

$(TMP_DIR)\jni.obj: jni.c
	$(CC) $(CFLAGS) $(JNI_INC) /Fo$@ /c jni.c

$(TMP_DIR)\cxx\Awk.obj: Awk.cpp Awk.hpp
	$(CXX) $(CXXFLAGS) /Fo$@ /c Awk.cpp

$(TMP_DIR)\cxx\StdAwk.obj: StdAwk.cpp StdAwk.hpp Awk.hpp
	$(CXX) $(CXXFLAGS) /Fo$@ /c StdAwk.cpp

$(TMP_DIR)\ase\awk\Awk.class: Awk.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Awk.java

$(TMP_DIR)\ase\awk\StdAwk.class: StdAwk.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) StdAwk.java

$(TMP_DIR)\ase\awk\Context.class: Context.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Context.java

$(TMP_DIR)\ase\awk\Extio.class: Extio.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Extio.java

$(TMP_DIR)\ase\awk\IO.class: IO.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) IO.java

$(TMP_DIR)\ase\awk\Console.class: Console.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Console.java

$(TMP_DIR)\ase\awk\File.class: File.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) File.java

$(TMP_DIR)\ase\awk\Pipe.class: Pipe.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Pipe.java

$(TMP_DIR)\ase\awk\Exception.class: Exception.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Exception.java

$(OUT_DIR):
	-md $(OUT_DIR)

$(TMP_DIR):
	-md $(TMP_DIR)

$(TMP_DIR_CXX): $(TMP_DIR)
	-md $(TMP_DIR_CXX)

clean:
	-del $(OUT_FILE_LIB) 
	-del $(OUT_FILE_JNI) 
	-del $(OUT_FILE_JAR) 
	-del $(OUT_FILE_LIB_CXX) 
	-del $(OBJ_FILES_LIB) 
	-del $(OBJ_FILES_JNI) 
	-del $(OBJ_FILES_JAR) 
	-del $(OBJ_FILES_LIB_CXX)

