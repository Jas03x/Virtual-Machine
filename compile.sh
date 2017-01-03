INCLUDE_PATHS="-I/Library/Frameworks/SDL2.framework/Versions/A/Headers/ "
FRAMEWORK_PATHS="-F/Library/Frameworks/ "
FRAMEWORKS="-framework SDL2 "
FRAMEWORKS+="-framework OpenGL "
FRAMEWORKS+="-framework CoreFoundation "
gcc -Wall $* $INCLUDE_PATHS$FRAMEWORK_PATHS $FRAMEWORKS machine.c -o machine
gcc -Wall $* compiler.c -o compiler
