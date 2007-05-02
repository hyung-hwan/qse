cd ..\..\..
move ase\test\awk\AwkApplet*.class .
jar cvf AwkApplet.jar *.class ase\test\awk\*.class ase\awk\*.class
move AwkApplet.jar ase\test\awk
cd ase\test\awk

copy ..\..\awk\aseawk_jni.dll .
jarsigner -keystore ase.store AwkApplet.jar asecert	
