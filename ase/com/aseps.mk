
aseps.dll: dlldata.obj ase_p.obj ase_i.obj
	link /dll /out:aseps.dll /def:aseps.def /entry:DllMain dlldata.obj ase_p.obj ase_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del aseps.dll
	@del aseps.lib
	@del aseps.exp
	@del dlldata.obj
	@del ase_p.obj
	@del ase_i.obj
