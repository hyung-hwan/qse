function ip2int(ip) {                                
	ret=0;                  
	n=split(ip,a,".");
	for (x=1;x<=n;x++) ret= ((ret << 8) | a[x])          
	return ret                               
}

function int2ip(ip,         ret,x) {
	ret = ip & 255;
	ip = ip >> 8;
	for(;x<3;x++) {                                  
		ret=(ip & 255)"."ret;
		ip=ip >> 8;
	}                                     
	return ret
} 

BEGIN {
	print int2ip(ip2int("255.255.255.0"));
	print int2ip(ip2int("255.0.255.0"));
	print int2ip(ip2int("127.0.0.1"));
	print int2ip(ip2int("192.168.1.1"));
}

