function sum(i,	k, y)
{

    y = 0;
    for (k = i; k; k = k - 1)
    {
        y = y + k;
    }

    return y;
    y = 10;
    return y;
}

BEGIN {
   /*s = sum(10000000);*/
   s = sum (100);
}

