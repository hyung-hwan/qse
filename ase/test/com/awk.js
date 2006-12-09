var awk, first, n

first = true

function awk_OpenSource (mode)
{
	WScript.echo ("OpenSource - mode:" + mode);
	return 1;
}

function awk_CloseSource (mode)
{
	WScript.echo ("CloseSource - mode:" + mode);
	return 0;
}

function awk_ReadSource (buf)
{
	WScript.echo ("ReadSource - buf: [" + buf.Value + "]");
	if (first)
	{
		buf.Value = "BEGIN {print 1; print 2; print 3 > \"x\";}"
		first = false
		return buf.Value.length;
	}
	else return 0;
}

function awk_WriteSource (buf)
{
	//WScript.echo ("WriteSource - cnt: " + cnt)
	WScript.echo (buf.Value);
	return buf.Value.length;
}

awk = WScript.CreateObject("ASE.Awk");
WScript.ConnectObject (awk, "awk_");


n = awk.Parse();
if (n == -1)
{
	WScript.echo ("parse failed");
	WScript.quit (1);
}

n = awk.Run ();
if (n == -1)
{
	WScript.echo ("run failed");
	WScript.quit (1);
}


