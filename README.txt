ELF patching tool that strips versioned symbol information from dynamically
linked binaries.

Use this to build a binary with a new toolchain that can run on older systems.

`objcopy --remove-section .gnu.version --remove-section .gnu.version_r` should
be used to remove symbol versioning sections from your binary.
`strip-versioned-symbols` removes the remaining references to these sections
from the .dynamic section by removing all DT_VERSYM, DT_VERNEED, and
DT_VERNEEDNUM entries.

Removing these sections without removing the .dynamic entries that point to
them will make ld.so complain with "unsupported version 0 of Verneed record" or
similar; ld.so will still follow the file offset listed in DT_VERNEED even if
there are no sections of type `SHT_GNU_verneed`.

Some symbol versioning detritus (e.g. strings that look like "GLIBC_2.2.5")
will remain in your binary's .dynstr section.

Link your binaries with `-Wl,--hash-style=both` for compatibility with really
old glibc (pre-871b91589bf4f6dfe19d5987b0a05bd7cf936ecc, around 2006-06-10)
versions that don't support DT_GNU_HASH-only binaries.

ELF symbol versioning is a misfeature. Existing workarounds for building
executables using new toolchains while maintaining compatibility with older
systems are ludicrous and labor-intensive. As far as I can tell, neither strip
nor objcopy allow you to remove DT_VERSYM, DT_VERNEED, or DT_VERNEEDNUM. And
neither ld nor lld let you opt out of using symbol versioning.

Note that, when resolving an unversioned symbol, the behavior of glibc's ld.so
is not the same as the behavior of `dlsym`. `dlsym` ostensibly finds the
"default" versioned symbol (the thing with a '@@', rather than a '@', as in
@@GLIBC_WHY_DID_THEY_DO_THIS) while ld.so is supposed find the "earliest"
versioned symbol. See 2b and 5 in
https://sourceware.org/pipermail/libc-alpha/2017-April/080127.html and see
https://sourceware.org/pipermail/libc-alpha/2017-April/080160.html. In reality
something weirder happens:
https://sourceware.org/bugzilla/show_bug.cgi?id=12977.

This means that if, for whatever reason, you're stuck using POSIX condition
variables, you won't have access to `CLOCK_MONOTONIC` timeouts via
`pthread_condattr_setclock` unless you use `dlsym` to find `pthread_cond_init`
and the related functions.

Thanks to Mark Laws (https://github.com/drvink/) for his knowledge of ELF
esoterica and for random related conversations.

Also see:
- https://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Core-generic/LSB-Core-generic/symversion.html
- https://guru.multimedia.cx/ld-so-gnu-linkerloader/
