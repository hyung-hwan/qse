NAME = aseawk

!ifndef MODE
MODE = release
!endif

JNI_INC = \
	-I"$(JAVA_HOME)\include" \
	-I"$(JAVA_HOME)\include\win32" 

CC = bcc32 
CXX = bcc32
LD = ilink32
AR = tlib 
JAVAC = javac
JAR = jar

CFLAGS = -O2 -WM -WU -RT- -w -q -I..\..
CXXFLAGS = -O2 -WM -WU -RT- -w -q -I..\..
JAVACFLAGS = -classpath ..\.. -Xlint:unchecked

LDFLAGS = -Tpd -ap -Gn -c -q -L..\$(MODE)\lib
STARTUP = c0d32w.obj
LIBS = import32.lib cw32mt.lib asecmn.lib aseutl.lib $(NAME).lib

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
	$(TMP_DIR)\ase\awk\Clearable.class \
	$(TMP_DIR)\ase\awk\Argument.class \
	$(TMP_DIR)\ase\awk\Return.class \
	$(TMP_DIR)\ase\awk\Extio.class \
	$(TMP_DIR)\ase\awk\IO.class \
	$(TMP_DIR)\ase\awk\Console.class \
	$(TMP_DIR)\ase\awk\File.class \
	$(TMP_DIR)\ase\awk\Pipe.class \
	$(TMP_DIR)\ase\awk\Exception.class

all: lib

lib: $(TMP_DIR) $(OUT_DIR) $(OUT_DIR_CXX) $(OUT_FILE_LIB) $(OUT_FILE_LIB_CXX)

jnidll: $(TMP_DIR) $(OUT_DIR) $(OUT_FILE_JNI)

jar: $(OUT_FILE_JAR)

$(OUT_FILE_LIB): $(OBJ_FILES_LIB)
	$(AR) $(OUT_FILE_LIB) @&&!
+-$(**: = &^
+-)
!

$(OUT_FILE_LIB_CXX): $(OBJ_FILES_LIB_CXX)
	$(AR) "$(OUT_FILE_LIB_CXX)" @&&!
+-$(**: = &^
+-)
!

$(OUT_FILE_JNI): $(OUT_FILE_LIB) $(OBJ_FILES_JNI)
	$(LD) $(LDFLAGS) $(STARTUP) $(OBJ_FILES_JNI),$(OUT_FILE_JNI),,$(LIBS),jni.def,

$(OUT_FILE_JAR): $(OBJ_FILES_JAR)
	$(JAR) -Mcvf $(OUT_FILE_JAR) -C $(TMP_DIR) ase

$(TMP_DIR)\awk.obj: awk.c
	$(CC) $(CFLAGS) -o$@ -c awk.c

$(TMP_DIR)\err.obj: err.c
	$(CC) $(CFLAGS) -o$@ -c err.c

$(TMP_DIR)\tree.obj: tree.c
	$(CC) $(CFLAGS) -o$@ -c tree.c

$(TMP_DIR)\tab.obj: tab.c
	$(CC) $(CFLAGS) -o$@ -c tab.c

$(TMP_DIR)\map.obj: map.c
	$(CC) $(CFLAGS) -o$@ -c map.c

$(TMP_DIR)\parse.obj: parse.c
	$(CC) $(CFLAGS) -o$@ -c parse.c

$(TMP_DIR)\run.obj: run.c
	$(CC) $(CFLAGS) -o$@ -c run.c

$(TMP_DIR)\rec.obj: rec.c
	$(CC) $(CFLAGS) -o$@ -c rec.c

$(TMP_DIR)\val.obj: val.c
	$(CC) $(CFLAGS) -o$@ -c val.c

$(TMP_DIR)\func.obj: func.c
	$(CC) $(CFLAGS) -o$@ -c func.c

$(TMP_DIR)\misc.obj: misc.c
	$(CC) $(CFLAGS) -o$@ -c misc.c

$(TMP_DIR)\extio.obj: extio.c
	$(CC) $(CFLAGS) -o$@ -c extio.c

$(TMP_DIR)\rex.obj: rex.c
	$(CC) $(CFLAGS) -o$@ -c rex.c

$(TMP_DIR)\jni.obj: jni.c
	$(CC) $(CFLAGS) $(JNI_INC) -o$@ -c jni.c

$(TMP_DIR)\cxx\Awk.obj: Awk.cpp Awk.hpp
	$(CXX) $(CXXFLAGS) -o$@ -c Awk.cpp

$(TMP_DIR)\cxx\StdAwk.obj: StdAwk.cpp StdAwk.hpp Awk.hpp
	$(CXX) $(CXXFLAGS) -o$@ -c StdAwk.cpp

$(TMP_DIR)\ase\awk\Awk.class: Awk.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Awk.java

$(TMP_DIR)\ase\awk\StdAwk.class: StdAwk.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) StdAwk.java

$(TMP_DIR)\ase\awk\Context.class: Context.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Context.java

$(TMP_DIR)\ase\awk\Clearable.class: Clearable.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Clearable.java

$(TMP_DIR)\ase\awk\Argument.class: Argument.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Argument.java

$(TMP_DIR)\ase\awk\Return.class: Return.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR) Return.java

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
