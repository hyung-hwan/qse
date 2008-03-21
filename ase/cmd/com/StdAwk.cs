/*
 * $Id: StdAwk.cs,v 1.2 2007/09/18 14:30:41 bacon Exp $
 */

using System;
using System.Collections.Generic;
using System.Text;

namespace ase.com
{
	public class StdAwk: Awk
	{
		public StdAwk(): base ()
		{
			AddFunction("sin", 1, 1, new FunctionHandler(handleSin));
			AddFunction("cos", 1, 1, new FunctionHandler(handleCos));
			AddFunction("tan", 1, 1, new FunctionHandler(handleTan));
		}

		protected virtual object handleSin(object[] args)
		{
			if (args[0] is System.Double)
			{
				return System.Math.Sin((double)args[0]);
			}
			else if (args[0] is System.Int32)
			{
				return System.Math.Sin((double)(int)args[0]);
			}
			else if (args[0] is System.Int64)
			{
				return System.Math.Sin((double)(long)args[0]);
			}
			else if (args[0] is string)
			{
				double t;

				/* TODO: atoi */
				try { t = System.Double.Parse((string)args[0]); }
				catch (System.Exception) { t = 0; }

				return System.Math.Sin(t);
			}
			else
			{
				return System.Math.Sin(0.0);
			}
		}

		protected virtual object handleCos(object[] args)
		{
			return 0;
		}

		protected virtual object handleTan(object[] args)
		{
			return 0;
		}		
	}
}
