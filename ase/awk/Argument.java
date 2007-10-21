/*
 * $Id: Argument.java,v 1.5 2007/10/19 15:02:33 bacon Exp $
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

	public boolean isIndexed ()
	{
		return isindexed (this.runid, this.valid);
	}

	public Argument getIndexed (String idx) throws Exception
	{
		return getindexed (this.runid, this.valid, idx);
	}

	public Argument getIndexed (long idx) throws Exception
	{
		return getIndexed (Long.toString(idx));
	}

	protected native long getintval (long runid, long valid);
	protected native double getrealval (long runid, long valid);
	protected native String getstrval (long runid, long valid) throws Exception;
	protected native boolean isindexed (long runid, long valid);
	protected native Argument getindexed (long runid, long valid, String idx) throws Exception;
}
