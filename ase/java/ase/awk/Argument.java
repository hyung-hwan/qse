/*
 * $Id: Argument.java 183 2008-06-03 08:18:55Z baconevi $
 *
 * {License}
 */

package ase.awk;

public class Argument implements Clearable
{
	protected long runid;
	//protected long valid;
	public long valid;

	/* An instance of the Argument class should not be used
	 * outside the context where it is availble. When it is
	 * referenced that way, the getXXX methods may cause
	 * JVM to crash */

	Argument (Context ctx, long runid, long valid)
	{
		if (ctx != null) ctx.pushClearable (this);
		this.runid = runid;
		this.valid = valid;
	}

	Argument (long runid, long valid)
	{
		this (null, runid, valid);
	}

	public void clear ()
	{
		clearval (this.runid, this.valid);
	}

	public long getIntValue ()
	{
		if (this.valid == 0) return 0;
		return getintval (this.runid, this.valid);
	}

	public double getRealValue ()
	{
		if (this.valid == 0) return 0.0;
		return getrealval (this.runid, this.valid);
	}

	public String getStringValue () throws Exception
	{
		if (this.valid == 0) return "";
		return getstrval (this.runid, this.valid);
	}

	public boolean isIndexed ()
	{
		if (this.valid == 0) return false;
		return isindexed (this.runid, this.valid);
	}

	public Argument getIndexed (String idx) throws Exception
	{
		if (this.valid == 0) return null;
		return getindexed (this.runid, this.valid, idx);
	}

	public Argument getIndexed (long idx) throws Exception
	{
		if (this.valid == 0) return null;
		return getIndexed (Long.toString(idx));
	}

	protected native long getintval (long runid, long valid);
	protected native double getrealval (long runid, long valid);
	protected native String getstrval (long runid, long valid) throws Exception;
	protected native boolean isindexed (long runid, long valid);
	protected native Argument getindexed (long runid, long valid, String idx) throws Exception;

	protected native void clearval (long runid, long valid);
}
