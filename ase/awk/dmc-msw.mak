NAME = aseawk
JNI =

JNI_INC = \
	-I"$(JAVA_HOME)/include" \
	-I"$(JAVA_HOME)/include\win32" 

CC = dmc
CXX = dmc
LD = link
AR = lib
JAVAC = javac

CFLAGS = -mn -I..\.. $(JNI_INC) 
CXXFLAGS = -Aa -Ab -Ae -mn -I..\.. $(JNI_INC) 
JAVACFLAGS = -classpath ..\.. -Xlint:unchecked

MODE=debug

OUT_DIR = ..\$(MODE)\lib
OUT_FILE_LIB = $(OUT_DIR)\$(NAME).lib
OUT_FILE_JNI = $(OUT_DIR)\lib$(NAME)_jni.la
OUT_FILE_LIB_CXX = $(OUT_DIR)\$(NAME)pp.lib
OUT_FILE_JAR = $(OUT_DIR)/$(NAME).jar

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
	$(TMP_DIR)/ase/awk/Awk.class \
	$(TMP_DIR)/ase/awk/StdAwk.class \
	$(TMP_DIR)/ase/awk/Extio.class \
	$(TMP_DIR)/ase/awk/IO.class \
	$(TMP_DIR)/ase/awk/Console.class \
	$(TMP_DIR)/ase/awk/File.class \
	$(TMP_DIR)/ase/awk/Pipe.class \
	$(TMP_DIR)/ase/awk/Exception.class

lib: build$(JNI)

build: $(OUT_FILE_LIB) $(OUT_FILE_LIB_CXX)

buildjni: build $(OUT_FILE_JNI)

$(OUT_FILE_LIB): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_LIB)
	$(AR) -c $(OUT_FILE_LIB) $(OBJ_FILES_LIB)

$(OUT_FILE_LIB_CXX): $(TMP_DIR_CXX) $(OUT_DIR) $(OUT_FILE_LIB) $(OBJ_FILES_LIB_CXX)
	$(AR) -c $(OUT_FILE_LIB_CXX) $(OBJ_FILES_LIB_CXX)

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
	$(CC) $(CFLAGS) $(CFLAGS_JNI) -o $@ -c jni.c

$(TMP_DIR)\cxx\Awk.obj: Awk.cpp Awk.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c Awk.cpp

$(TMP_DIR)\cxx\StdAwk.obj: StdAwk.cpp StdAwk.hpp Awk.hpp
	$(CXX) $(CXXFLAGS) -o $@ -c StdAwk.cpp

$(TMP_DIR)/ase/awk/Awk.class: Awk.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) Awk.java

$(TMP_DIR)/ase/awk/StdAwk.class: StdAwk.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) StdAwk.java

$(TMP_DIR)/ase/awk/Extio.class: Extio.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) Extio.java

$(TMP_DIR)/ase/awk/IO.class: IO.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) IO.java

$(TMP_DIR)/ase/awk/Console.class: Console.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) Console.java

$(TMP_DIR)/ase/awk/File.class: File.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) File.java

$(TMP_DIR)/ase/awk/Pipe.class: Pipe.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) Pipe.java

$(TMP_DIR)/ase/awk/Exception.class: Exception.java
	$(JAVAC) -classpath ../.. -d $(TMP_DIR) Exception.java

$(OUT_DIR):
	md $(OUT_DIR)

$(TMP_DIR):
	md $(TMP_DIR)

$(TMP_DIR_CXX): $(TMP_DIR)
	md $(TMP_DIR_CXX)

clean:
	rm -rf $(OUT_FILE_LIB) $(OUT_FILE_JNI) $(OUT_FILE_JAR) $(OUT_FILE_LIB_CXX) $(OBJ_FILES_LIB) $(OBJ_FILES_JNI) $(OBJ_FILES_JAR) $(OBJ_FILES_LIB_CXX)

