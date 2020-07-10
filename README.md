# what is wrapcc

wrapcc is gcc like wraper for building cross host targets. It help make life easier to build something for IOS, Android, Openwrt, etc.

# Usage

* copy wrapcc to somewhere (like ~/bin) rename it to `aarch64-apple-darwin-gcc` .

* Edit config file `~/bin/aarch64-apple-darwin-gcc.wrapcc.cfg` to something like:

```
#!/bin/bash
CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
COMMON_FLAGS="-arch arm64 -miphoneos-version-min=8.0 -g"
COMMON_FLAGS+=" -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk"
COMMON_FLAGS+=" -F/Users/xungeng/work/ios_research/ios_frameworks"

CFLAGS="$COMMON_FLAGS -fmessage-length=0 -fdiagnostics-show-note-include-stack -fmacro-backtrace-limit=0 -fobjc-arc -gmodules
 -Wno-trigraphs -fpascal-strings -Wno-missing-field-initializers -Wno-missing-prototypes -Wno-implicit-atomic-properties
 -Wno-arc-repeated-use-of-weak -Wduplicate-method-match -Wno-missing-braces -Wparentheses -Wswitch -Wunused-function -Wno-unused-label
 -Wno-unused-parameter -Wunused-variable -Wunused-value -Wempty-body -Wuninitialized -Wno-unknown-pragmas -Wno-shadow
 -Wno-four-char-constants -Wno-conversion -Wconstant-conversion -Wint-conversion -Wbool-conversion -Wenum-conversion
 -Wno-newline-eof -Wno-selector -Wno-strict-selector-match -Wundeclared-selector -Wno-deprecated-implementations -Wno-nullability-completeness
 -fstrict-aliasing -Wprotocol -Wdeprecated-declarations -fvisibility=hidden -Wno-sign-conversion"
CFLAGS+=" -I/Users/xungeng/work/ios_research/ios_inc"

CXXFLAGS="$CFLAGS -std=gnu++11 -stdlib=libc++
-Wc++11-extensions
-Wno-non-virtual-dtor
-Wno-overloaded-virtual
-Wno-exit-time-destructors
-Winvalid-offsetof
-fvisibility-inlines-hidden
"

LIBDIRS="-L/Users/xungeng/work/ios_research/ios_lib"

WRAPCC_LDFLAGS="$COMMON_FLAGS $LIBDIRS -dead_strip"
WRAPCC_CFLAGS="$CFLAGS -std=c99"
WRAPCC_CXXFLAGS=$CXXFLAGS
WRAPCC_CC=$CC

WRAPCC_DBG=0
WRAPCC_CFLAGS2= # -O0

WRAPCC_REMOVE="-Werror"
```

**Note:**

This is **NOT** a bash script but bash-like. The tag `#!/bin/bash` is just for making vim happy to make it easier to edit this config file.

* try `aarch64-apple-darwin-gcc` for fun.

Ex: `aarch64-apple-darwin-gcc -o main main.c` and `main.c` has contents like:

```
#include <stdio.h>
int main()
{
	printf("hello\n");
	return 0;
}
```
