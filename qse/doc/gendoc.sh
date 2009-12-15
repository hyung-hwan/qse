#!/bin/sh

SED="../test/sed/sed01"

"${SED}" '/^OUTPUT_LANGUAGE[[:space:]]*=/s/English/Korean/
	/^OUTPUT_DIRECTORY[[:space:]]*=/s/qse/qse.ko/
	/^INPUT[[:space:]]*=/s/page/page.ko/' < Doxyfile > Doxyfile.ko

#"${SED}" '/^OUTPUT_LANGUAGE[[:space:]]*=/s/English/Chinese/
#	/^OUTPUT_DIRECTORY[[:space:]]*=/s/qse/qse.cn/
#	/^INPUT[[:space:]]*=/s/page/page.cn/' < Doxyfile > Doxyfile.cn

#"${SED}" '/^OUTPUT_LANGUAGE[[:space:]]*=/s/English/Japanese/
#	/^OUTPUT_DIRECTORY[[:space:]]*=/s/qse/qse.ja/
#	/^INPUT[[:space:]]*=/s/page/page.ja/' < Doxyfile > Doxyfile.ja

doxygen Doxyfile
#doxygen Doxyfile.ko
#doxygen Doxyfile.cn
#doxygen Doxyfile.ja
