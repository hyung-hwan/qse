/*
 * $Id: Argument.java,v 1.4 2007/10/19 03:50:32 bacon Exp $
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

	public long getIntValue ()
	{
		return getintval (this.runid, this.valid);
	}

	public double getRealValue ()
	{
		return getrealval (this.runid, this.valid);
	}

	public String getStringValue () throws Exception
	{
		return getstrval (this.runid, this.valid);
	}

	public Argument getIndexed (String idx)
	{
		// TODO:..
		return null;
	}

	public Argument getIndexed (long idx)
	{
		return getIndexed (Long.toString(idx));
	}

	protected native long getintval (long runid, long valid);
	protected native double getrealval (long runid, long valid);
	protected native String getstrval (long runid, long valid) throws Exception;
}
