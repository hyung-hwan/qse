# 
# author: pynoos @ kldp
#
/^linux/ { 
:grab
	/^}/ b end
	s/^Host:.*$/HOST: com.com/
	s/^Address:.*$/ADDRESS: 45.34.34.33/
	n       
	b grab  
:end
	n       
	p       
}
