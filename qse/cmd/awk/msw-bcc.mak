
# ilink32.exe     link.exe
# -aa             /subsystem:windows
# -ap             /subsystem:console
# -ad             /subsystem:native
#
# -Tpe            
# -Tpd            /dll
NAME = aseawk

!ifndef MODE
MODE = release
!endif

CC = bcc32
CXX = bcc32
LD = ilink32
JAVAC = javac
JAR = jar

CFLAGS = -WM -WU -RT- -w -q -I..\..\.. 
CXXFLAGS = -WM -WU -RT- -w -q -I..\..\.. 
LDFLAGS = -ap -Tpe -Gn -c -q -L..\..\$(MODE)\lib -L\progra~1\borland\bds\4.0\lib
LIBS = asecmn.lib aseawk.lib aseutl.lib import32.lib cw32mt.lib
LIBS_CXX = $(LIBS) "aseawk++.lib"
STARTUP = c0x32w.obj

JAVACFLAGS = -classpath ..\..\$(MODE)\lib\aseawk.jar;. -Xlint:unchecked

OUT_DIR = ..\..\$(MODE)\bin
OUT_FILE_BIN = $(OUT_DIR)\$(NAME).exe
OUT_FILE_BIN_CXX = "$(OUT_DIR)\$(NAME)++.exe"
OUT_FILE_JAR = $(OUT_DIR)\$(NAME).jar

TMP_DIR = $(MODE)
TMP_DIR_CXX = $(TMP_DIR)\cxx
TMP_DIR_JAR = $(TMP_DIR)\java

OBJ_FILES_BIN = $(TMP_DIR)\awk.obj
OBJ_FILES_BIN_CXX = $(TMP_DIR_CXX)\Awk.obj

OBJ_FILES_JAR = \
	$(TMP_DIR_JAR)\AseAwk.class \
	$(TMP_DIR_JAR)\AseAwkPanel.class \
	$(TMP_DIR_JAR)\AseAwkApplet.class

TARGETS = bin

!if "$(JAVA_HOME)" != ""
TARGETS = $(TARGETS) jar
!endif

!IF "$(MODE)" == "debug"
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG 
CXXFLAGS = $(CXXFLAGS) -D_DEBUG -DDEBUG
!ELSEIF "$(MODE)" == "release"
CFLAGS = $(CFLAGS) -DNDEBUG -O2
CXXFLAGS = $(CXXFLAGS) -DNDEBUG -O2
!ELSE
CFLAGS = $(CFLAGS) 
CXXFLAGS = $(CXXFLAGS)
!ENDIF

all: $(TARGETS)

bin: $(OUT_FILE_BIN) $(OUT_FILE_BIN_CXX)

jar: $(OUT_FILE_JAR)

$(OUT_FILE_BIN): $(TMP_DIR) $(OUT_DIR) $(OBJ_FILES_BIN)
	$(LD) $(LDFLAGS) $(STARTUP) $(OBJ_FILES_BIN),$@,,$(LIBS),,

$(OUT_FILE_BIN_CXX): $(TMP_DIR_CXX) $(OUT_FILE_BIN) $(OBJ_FILES_BIN_CXX)
	$(LD) $(LDFLAGS) $(STARTUP) $(OBJ_FILES_BIN_CXX),$@,,$(LIBS_CXX),,

$(OUT_FILE_JAR): $(TMP_DIR_JAR) $(OBJ_FILES_JAR)
	$(JAR) -xvf ..\..\$(MODE)\lib\aseawk.jar
	$(JAR) -cvfm $(OUT_FILE_JAR) manifest ase -C $(TMP_DIR_JAR) .

$(TMP_DIR)\awk.obj: awk.c
	$(CC) $(CFLAGS) -o$@ -c awk.c

$(TMP_DIR_CXX)\Awk.obj: Awk.cpp
	$(CC) $(CXXFLAGS) -o$@ -c Awk.cpp

$(TMP_DIR_JAR)\AseAwk.class: AseAwk.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR_JAR) AseAwk.java	

$(TMP_DIR_JAR)\AseAwkApplet.class: AseAwkApplet.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR_JAR) AseAwkApplet.java

$(TMP_DIR_JAR)\AseAwkPanel.class: AseAwkPanel.java
	$(JAVAC) $(JAVACFLAGS) -d $(TMP_DIR_JAR) AseAwkPanel.java

$(OUT_DIR):
	-md $(OUT_DIR)

$(TMP_DIR):
	-md $(TMP_DIR)

$(TMP_DIR_CXX): $(TMP_DIR)
	-md $(TMP_DIR_CXX)

$(TMP_DIR_JAR): $(TMP_DIR)
	-md $(TMP_DIR_JAR)

clean:
	-del $(OUT_FILE_BIN) 
	-del $(OUT_FILE_BIN_CXX) 
	-del $(OBJ_FILES_BIN) 
	-del $(OBJ_FILES_BIN_CXX) 
	-del $(OUT_FILE_JAR)
	-del $(OBJ_FILES_JAR)
	-del $(TMP_DIR)\*.class

