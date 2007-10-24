/*
 * $Id: Argument.java,v 1.6 2007/10/21 13:58:47 bacon Exp $
 */

package ase.awk;

public class Argument
{
	protected long runid;
	protected long valid;

	/* An instance of the Argument class should not be used
	 * outside the context where it is availble. When it is
	 * referenced that way, the getXXX methods may cause
	 * JVM to crash */

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
