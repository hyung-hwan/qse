/*
 * $Id: Argument.java,v 1.2 2007/10/17 14:38:28 bacon Exp $
 */

package ase.awk;

public class Argument
{
	protected Context ctx;
	protected long value;

	Argument (Context ctx)
	{
		this.ctx = ctx;
	}

	long getIntValue ()
	{
		return getintval (ctx.getId(), value);
	}

	double getRealValue ()
	{
		return 0.0;
	}

	String getStringValue ()
	{
		return null;
	}

	Argument getIndexed (String idx)
	{
		return null;
	}

	Argument getIndexed (long idx)
	{
		return getIndexed (Long.toString(idx));
	}

	protected native long getintval (long runid, long valid);
	protected native double getrealval (long runid, long valid);
	protected native String getstrval (long runid, long valid);
}
