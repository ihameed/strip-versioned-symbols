#!/bin/sh

set -e

clang++ -g -O2 \
    -fno-exceptions -fno-rtti -fno-plt -fno-semantic-interposition \
    -ffunction-sections -fdata-sections \
    -Wl,--gc-sections -Wl,-Bsymbolic -Wl,-z,relro -Wl,-z,now \
    -Wl,--no-undefined -Wl,--as-needed \
    -Wl,--hash-style=both \
    -o strip-versioned-symbols \
    strip-versioned-symbols.cpp

cp strip-versioned-symbols strip-versioned-symbols.tmp

./strip-versioned-symbols.tmp strip-versioned-symbols

objcopy --remove-section .gnu.version --remove-section .gnu.version_r strip-versioned-symbols

rm strip-versioned-symbols.tmp
