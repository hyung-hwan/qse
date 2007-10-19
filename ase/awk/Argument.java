/*
 * $Id: Argument.java,v 1.3 2007/10/18 14:51:04 bacon Exp $
 */

package ase.awk;

public class Argument
{
	protected long runid;
	protected long valid;

	Argument (long runid, long valid)
	{
		this.runid = runid;
		this.valid = valid;
	}

	long getIntValue ()
	{
		return getintval (this.runid, this.valid);
	}

	double getRealValue ()
	{
		return getrealval (this.runid, this.valid);
	}

	String getStringValue () throws Exception
	{
		return getstrval (this.runid, this.valid);
	}

	Argument getIndexed (String idx)
	{
		// TODO:..
		return null;
	}

	Argument getIndexed (long idx)
	{
		return getIndexed (Long.toString(idx));
	}

	protected native long getintval (long runid, long valid);
	protected native double getrealval (long runid, long valid);
	protected native String getstrval (long runid, long valid) throws Exception;
}
