cd ..\..\..
move ase\cmd\awk\AwkApplet*.class .
jar cvf AwkApplet.jar *.class ase\cmd\awk\*.class ase\awk\*.class
move AwkApplet.jar ase\cmd\awk
cd ase\cmd\awk

copy ..\..\awk\aseawk_jni.dll .
jarsigner -keystore ase.store AwkApplet.jar asecert	
