/*
 * $Id: Return.java,v 1.4 2007/10/24 14:17:32 bacon Exp $
 */

package ase.awk;

public class Return
{
	protected long runid;
	protected long valid;

	/* An instance of the Return class should not be used
	 * outside the context where it is availble. When it is
	 * referenced that way, the setXXX methods may cause
	 * JVM to crash */

	Return (long runid, long valid)
	{
		this.runid = runid;
		this.valid = valid;
	}

	public boolean isIndexed ()
	{
		return isindexed (this.runid, this.valid);
	}

	public void setIntValue (long v)
	{
		setintval (this.runid, this.valid, v);
	}

	public void setIntValue (int v)
	{
		setintval (this.runid, this.valid, (long)v);
	}

	public void setIntValue (short v)
	{
		setintval (this.runid, this.valid, (long)v);
	}

	public void setIntValue (byte v)
	{
		setintval (this.runid, this.valid, (long)v);
	}

	public void setRealValue (double v)
	{
		setrealval (this.runid, this.valid, v);
	}

	public void setRealValue (float v)
	{
		setrealval (this.runid, this.valid, (double)v);
	}

	public void setStringValue (String v)
	{
		setstrval (this.runid, this.valid, v);
	}

	public void setIndexedIntValue (String index, long v)
	{
		setindexedintval (this.runid, this.valid, index, v);
	}

	public void setIndexedIntValue (String index, int v)
	{
		setindexedintval (this.runid, this.valid, index, (long)v);
	}

	public void setIndexedIntValue (String index, short v)
	{
		setindexedintval (this.runid, this.valid, index, (long)v);
	}

	public void setIndexedIntValue (String index, byte v)
	{
		setindexedintval (this.runid, this.valid, index, (long)v);
	}

	public void setIndexedRealValue (String index, double v)
	{
		setindexedrealval (this.runid, this.valid, index, v);
	}

	public void setIndexedRealValue (String index, float v)
	{
		setindexedrealval (this.runid, this.valid, index, (double)v);
	}

	public void setIndexedStringValue (String index, String v)
	{
		setindexedstrval (this.runid, this.valid, index, v);
	}

	public void setIndexedIntValue (long index, long v)
	{
		setindexedintval (this.runid, this.valid, Long.toString(index), v);
	}

	public void setIndexedIntValue (long index, int v)
	{
		setindexedintval (this.runid, this.valid, Long.toString(index), (long)v);
	}

	public void setIndexedIntValue (long index, short v)
	{
		setindexedintval (this.runid, this.valid, Long.toString(index), (long)v);
	}

	public void setIndexedIntValue (long index, byte v)
	{
		setindexedintval (this.runid, this.valid, Long.toString(index), (long)v);
	}

	public void setIndexedRealValue (long index, double v)
	{
		setindexedrealval (this.runid, this.valid, Long.toString(index), v);
	}

	public void setIndexedRealValue (long index, float v)
	{
		setindexedrealval (this.runid, this.valid, Long.toString(index), (double)v);
	}

	public void setIndexedStringValue (long index, String v)
	{
		setindexedstrval (this.runid, this.valid, Long.toString(index), v);
	}


	public void clear ()
	{
		clearval (this.runid, this.valid);
	}

	protected native boolean isindexed (long runid, long valid);

	protected native void setintval (long runid, long valid, long v);
	protected native void setrealval (long runid, long valid, double v);
	protected native void setstrval (long runid, long valid, String v);

	protected native void setindexedintval (
		long runid, long valid, String index, long v);
	protected native void setindexedrealval (
		long runid, long valid, String index, double v);
	protected native void setindexedstrval (
		long runid, long valid, String index, String v);

	protected native void clearval (long runid, long valid);
}
