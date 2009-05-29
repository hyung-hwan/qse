#!/bin/sh

SED="../test/sed/sed01"

"${SED}" '/^OUTPUT_LANGUAGE[[:space:]]*=/s/English/Korean/
	/^OUTPUT_DIRECTORY[[:space:]]*=/s/qse/qse.ko/
	/^INPUT[[:space:]]*=/s/page/page.ko/' < Doxyfile > Doxyfile.ko

doxygen Doxyfile
doxygen Doxyfile.ko
