dim awk, first

first = true

function awk_OpenSource (mode)
	WScript.echo ("OpenSource - mode:" & mode)
	awk_OpenSource = 1
end function

function awk_CloseSource (mode)
	WScript.echo ("CloseSource - mode:" & mode)
	awk_CloseSource = 0
end function

function awk_ReadSource (buf, cnt)
	WScript.echo ("ReadSource - cnt: " & cnt)
	if first then
		buf.Value = "BEGIN {print 1;}"
		first = false
		awk_ReadSource = len(buf.Value)
	else 
		awk_ReadSource = 0
	end if
end function

set awk = WScript.CreateObject("ASE.Awk")
call WScript.ConnectObject (awk, "awk_")

WScript.echo awk.Parse
set awk = nothing
