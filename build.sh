gcc father.c -o father
gcc son.c -o son
foo=`objdump -d son | grep ' <foo>:' | awk '{print $1}'`
./father son 0x$foo
