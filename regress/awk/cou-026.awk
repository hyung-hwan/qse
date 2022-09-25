BEGIN { FS = "\t"; }

      { pop[$4] += $3; }

END   { 

	count = 0;
	for (name in pop) 
	{
		for (i = 0; i < count; i++)
		{
			if (name < x[i]) 
			{
				for (j = count; j > i; j--)
				{
					x[j] = x[j-1];
					y[j] = y[j-1];
				}
				break;
			}
		}
			
		x[i] = name;
		y[i] = pop[name];
		count++;
	}


	for (i = 0; i < count; i++)
	{
		print x[i], y[i];
	}

}
