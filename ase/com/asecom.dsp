# Microsoft Developer Studio Project File - Name="asecom" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=asecom - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "asecom.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "asecom.mak" CFG="asecom - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "asecom - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "asecom - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "asecom - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../debug"
# PROP Intermediate_Dir "debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /ZI /Od /I "../.." /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"debug/ase.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 aseawk.lib asecmn.lib aseutl.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /debug /machine:I386 /implib:"debug/ase.lib" /pdbtype:sept /libpath:"$(OutDir)"
# Begin Custom Build - Performing registration
IntDir=.\debug
OutDir=.\../debug
TargetPath=\projects\ase\debug\ase.dll
InputPath=\projects\ase\debug\ase.dll
SOURCE="$(InputPath)"

"$(IntDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\ase.tlb "$(OUTDIR)\ase.tlb" 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(IntDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register DLL on Windows 95 
	:end 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "asecom - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../release"
# PROP Intermediate_Dir "release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /D "_ATL_MIN_CRT" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /W3 /O1 /I "../.." /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_USRDLL" /D "_UNICODE" /D "_ATL_STATIC_REGISTRY" /Yu"stdafx.h" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"release/awk.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 aseawk.lib asecmn.lib aseutl.lib  kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /machine:I386 /implib:"release/ase.lib" /libpath:"$(OutDir)"
# Begin Custom Build - Performing registration
IntDir=.\release
OutDir=.\../release
TargetPath=\projects\ase\release\ase.dll
InputPath=\projects\ase\release\ase.dll
SOURCE="$(InputPath)"

"$(IntDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\ase.tlb "$(OUTDIR)\ase.tlb" 
	if "%OS%"=="" goto NOTNT 
	if not "%OS%"=="Windows_NT" goto NOTNT 
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(IntDir)\regsvr32.trg" 
	goto end 
	:NOTNT 
	echo Warning : Cannot register DLL on Windows 95 
	:end 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "asecom - Win32 Debug"
# Name "asecom - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ase.cpp
# End Source File
# Begin Source File

SOURCE=.\ase.def
# End Source File
# Begin Source File

SOURCE=.\ase.idl
# ADD MTL /tlb ".\ase.tlb" /h "ase.h" /iid "ase_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\ase.rc
# End Source File
# Begin Source File

SOURCE=.\Awk.cpp
# End Source File
# Begin Source File

SOURCE=.\AwkExtio.cpp
# End Source File
# Begin Source File

SOURCE=.\Buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\stdafx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\Awk.h
# End Source File
# Begin Source File

SOURCE=.\awk_cp.h
# End Source File
# Begin Source File

SOURCE=.\AwkExtio.h
# End Source File
# Begin Source File

SOURCE=.\Buffer.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\stdafx.h
# End Source File
# Begin Source File

SOURCE=.\xxx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Awk.rgs
# End Source File
# Begin Source File

SOURCE=.\AwkExtio.rgs
# End Source File
# Begin Source File

SOURCE=.\Buffer.rgs
# End Source File
# End Group
# End Target
# End Project
