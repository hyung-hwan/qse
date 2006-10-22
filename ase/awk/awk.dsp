# Microsoft Developer Studio Project File - Name="awk" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=awk - Win32 Debug DLL
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "awk.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "awk.mak" CFG="awk - Win32 Debug DLL"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "awk - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "awk - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "awk - Win32 Debug DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "awk - Win32 Release DLL" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "awk - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MT /Za /W3 /GX /O2 /I "..\.." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\release\awk.lib"

!ELSEIF  "$(CFG)" == "awk - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MTd /Za /W3 /Gm /GX /ZI /Od /I "..\.." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\..\debug\awk.lib"

!ELSEIF  "$(CFG)" == "awk - Win32 Debug DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "awk___Win32_Debug_DLL"
# PROP BASE Intermediate_Dir "awk___Win32_Debug_DLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\.." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /FD /GZ /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MTd /Za /W3 /Gm /GX /ZI /Od /I "..\.." /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:console /dll /debug /machine:I386 /def:".\awk.def" /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /subsystem:console /dll /debug /machine:I386 /def:".\awk.def" /out:"..\..\debug\awk.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "awk - Win32 Release DLL"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "awk___Win32_Release_DLL"
# PROP BASE Intermediate_Dir "awk___Win32_Release_DLL"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /I "..\.." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /FD /c
# SUBTRACT BASE CPP /YX /Yc /Yu
# ADD CPP /nologo /MT /Za /W3 /GX /O2 /I "..\.." /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "_USRDLL" /FD /c
# SUBTRACT CPP /YX /Yc /Yu
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /version:0.1 /subsystem:console /dll /machine:I386 /def:".\awk.def"
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 /nologo /version:0.1 /subsystem:console /dll /machine:I386 /def:".\awk.def" /out:"..\..\release\awk.dll"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "awk - Win32 Release"
# Name "awk - Win32 Debug"
# Name "awk - Win32 Debug DLL"
# Name "awk - Win32 Release DLL"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\awk.c
# End Source File
# Begin Source File

SOURCE=.\awk.def

!IF  "$(CFG)" == "awk - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "awk - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "awk - Win32 Debug DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "awk - Win32 Release DLL"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\err.c
# End Source File
# Begin Source File

SOURCE=.\extio.c
# End Source File
# Begin Source File

SOURCE=.\func.c
# End Source File
# Begin Source File

SOURCE=.\map.c
# End Source File
# Begin Source File

SOURCE=.\misc.c
# End Source File
# Begin Source File

SOURCE=.\parse.c
# End Source File
# Begin Source File

SOURCE=.\rec.c
# End Source File
# Begin Source File

SOURCE=.\rex.c
# End Source File
# Begin Source File

SOURCE=.\run.c
# End Source File
# Begin Source File

SOURCE=.\str.c
# End Source File
# Begin Source File

SOURCE=.\tab.c
# End Source File
# Begin Source File

SOURCE=.\tree.c
# End Source File
# Begin Source File

SOURCE=.\val.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\awk.h
# End Source File
# Begin Source File

SOURCE=.\awk_i.h
# End Source File
# Begin Source File

SOURCE=.\extio.h
# End Source File
# Begin Source File

SOURCE=.\func.h
# End Source File
# Begin Source File

SOURCE=.\map.h
# End Source File
# Begin Source File

SOURCE=.\rex.h
# End Source File
# Begin Source File

SOURCE=.\run.h
# End Source File
# Begin Source File

SOURCE=.\str.h
# End Source File
# Begin Source File

SOURCE=.\tab.h
# End Source File
# Begin Source File

SOURCE=.\tree.h
# End Source File
# Begin Source File

SOURCE=.\val.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
