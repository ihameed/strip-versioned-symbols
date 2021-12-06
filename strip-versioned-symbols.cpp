/*
 * Copyright (c) 2021 Imran Hameed
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#define _FILE_OFFSET_BITS 64

#include <stdint.h>
#include <stddef.h> // size_t
#include <string.h> // memcpy

#include <algorithm> // std::remove_if


#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

namespace io {

enum struct seek_origin_t : uint8_t { Absolute, Relative, End };

auto const Absolute = seek_origin_t::Absolute;
auto const Relative = seek_origin_t::Relative;
auto const End = seek_origin_t::End;

struct read_t {
    void *env;
    size_t (* read) (void *dst, size_t sz, void *env);
    bool (* seek) (int64_t offset, seek_origin_t origin, void *env);
    int64_t (* curpos) (void *env);
};

struct readwrite_t {
    read_t read;
    size_t (* write) (void const *src, size_t sz, void *env);
    bool (* flush) (void *env);

    inline operator read_t const & () const { return read; }
};

size_t inline
read(read_t const &r, void * const dst, size_t const sz) {
    return r.read(dst, sz, r.env);
}

bool inline
seek(read_t const &r, int64_t const offset, seek_origin_t const origin) {
    return r.seek(offset, origin, r.env);
}

int64_t inline
curpos(read_t const &r) {
    return r.curpos(r.env);
}

size_t inline
write(readwrite_t const &r, void const * const src, size_t const sz) {
    return r.write(src, sz, r.read.env);
}

bool inline
flush(readwrite_t const &r) {
    return r.flush(r.read.env);
}

}

namespace elf {

size_t const EI_NIDENT = 16;

template <size_t bits>
struct inttypes;

template <>
struct inttypes<32> { using uint_t = uint32_t; using int_t = int32_t; };

template <>
struct inttypes<64> { using uint_t = uint64_t; using int_t = int64_t; };

template <size_t bits>
using Elf_Addr = typename inttypes<bits>::uint_t;

template <size_t bits>
using Elf_Off = typename inttypes<bits>::uint_t;

template <size_t bits>
using uint_t = typename inttypes<bits>::uint_t;

template <size_t bits>
using int_t = typename inttypes<bits>::int_t;

union ident_t {
    uint8_t raw[EI_NIDENT];
    struct {
        uint8_t magic_bytes[4];
        uint8_t elfclass;
        uint8_t elfdataformat;
        uint8_t elfversion;
        uint8_t osabi;
        uint8_t osabiversion;
        uint8_t _pad_[7];
    } f;
};

static_assert(sizeof(ident_t) == 16, "!");

template <size_t n>
struct header_t {
    ident_t e_ident;
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    Elf_Addr<n> e_entry;
    Elf_Off<n> e_phoff;
    Elf_Off<n> e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

static_assert(offsetof(header_t<64>, e_type) == offsetof(header_t<32>, e_type), "!");

struct header64_or_32_t {
    bool is_32;
    union {
        header_t<64> header64;
        header_t<32> header32;
    };
};

template <size_t n>
struct section_header_t {
    uint32_t sh_name;
    uint32_t sh_type;
    uint_t<n> sh_flags;
    Elf_Addr<n> sh_addr;
    Elf_Addr<n> sh_offset;
    uint_t<n> sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint_t<n> sh_addralign;
    uint_t<n> sh_entsize;
};

uint8_t const ELFCLASSNONE = 0;
uint8_t const ELFCLASS32 = 1;
uint8_t const ELFCLASS64 = 2;

uint8_t const ELFDATANONE = 0;
uint8_t const ELFDATA2LSB = 1;
uint8_t const ELFDATA2MSB = 2;

uint8_t const EV_NONE = 0;
uint8_t const EV_CURRENT = 1;

uint8_t const ELFOSABI_NONE = 0;

#define ELF_expand_as_uint32_constant(name, val) uint32_t const name = val;
#define ELF_expand_as_case(name, val) case name : return #name ;

#define ELF_section_header_types(_) \
    _(SHT_NULL, 0) \
    _(SHT_PROGBITS, 1) \
    _(SHT_SYMTAB, 2) \
    _(SHT_STRTAB, 3) \
    _(SHT_RELA, 4) \
    _(SHT_HASH, 5) \
    _(SHT_DYNAMIC, 6) \
    _(SHT_NOTE, 7) \
    _(SHT_NOBITS, 8) \
    _(SHT_REL, 9) \
    _(SHT_SHLIB, 10) \
    _(SHT_DYNSYM, 11) \
    _(SHT_NUM, 12) \
    _(SHT_INIT_ARRAY, 14) \
    _(SHT_FINI_ARRAY, 15) \
    _(SHT_GNU_VERNEED, 0x6ffffffe) \
    _(SHT_GNU_VERSYM, 0x6fffffff) \
    _(SHT_GNU_HASH, 0x6ffffff6) \
    _(SHT_LOPROC, 0x70000000) \
    _(SHT_HIPROC, 0x7fffffff) \
    _(SHT_LOUSER, 0x80000000) \
    _(SHT_HIUSER, 0xffffffff)

ELF_section_header_types(ELF_expand_as_uint32_constant)
char const *
string_of_elf_section_header_type(uint32_t const val) {
    switch (val) {
    ELF_section_header_types(ELF_expand_as_case)
    default: return "<unknown section header type>";
    }
}

template <size_t n>
struct dynamic_section_entry_t {
    int_t<n> d_tag;
    union {
        uint_t<n> d_val;
        Elf_Addr<n> d_ptr;
    } d_un;
};

static_assert(sizeof(dynamic_section_entry_t<32>) == 8, "!");
static_assert(sizeof(dynamic_section_entry_t<64>) == 16, "!");

#define ELF_dynamic_section_tags(_) \
    _(DT_NULL, 0) \
    _(DT_NEEDED, 1) \
    _(DT_PLTRELSZ, 2) \
    _(DT_PLTGOT, 3) \
    _(DT_HASH, 4) \
    _(DT_STRTAB, 5) \
    _(DT_SYMTAB, 6) \
    _(DT_RELA, 7) \
    _(DT_RELASZ, 8) \
    _(DT_RELAENT, 9) \
    _(DT_STRSZ, 10) \
    _(DT_SYMENT, 11) \
    _(DT_INIT, 12) \
    _(DT_FINI, 13) \
    _(DT_SONAME, 14) \
    _(DT_RPATH, 15) \
    _(DT_SYMBOLIC, 16) \
    _(DT_REL, 17) \
    _(DT_RELSZ, 18) \
    _(DT_RELENT, 19) \
    _(DT_PLTREL, 20) \
    _(DT_DEBUG, 21) \
    _(DT_TEXTREL, 22) \
    _(DT_JMPREL, 23) \
    _(DT_BIND_NOW, 24) \
    _(DT_INIT_ARRAY, 25) \
    _(DT_FINI_ARRAY, 26) \
    _(DT_INIT_ARRAYSZ, 27) \
    _(DT_FINI_ARRAYSZ, 28) \
    _(DT_RUNPATH, 29) \
    _(DT_FLAGS, 30) \
    _(DT_ENCODING, 32) \
    _(DT_LOOS, 0x6000000d) \
    _(DT_HIOS, 0x6ffff000) \
    _(DT_VALRNGLO, 0x6ffffd00) \
    _(DT_VALRNGHI, 0x6ffffdff) \
    _(DT_ADDRRNGLO, 0x6ffffe00) \
    _(DT_ADDRRNGHI, 0x6ffffeff) \
    _(DT_VERSYM, 0x6ffffff0) \
    _(DT_RELACOUNT, 0x6ffffff9) \
    _(DT_RELCOUNT, 0x6ffffffa) \
    _(DT_FLAGS_1, 0x6ffffffb) \
    _(DT_VERDEF, 0x6ffffffc) \
    _(DT_VERDEFNUM, 0x6ffffffd) \
    _(DT_VERNEED, 0x6ffffffe) \
    _(DT_VERNEEDNUM, 0x6fffffff) \
    _(DT_GNU_HASH, 0x6ffffef5) \
    _(DT_LOPROC, 0x70000000) \
    _(DT_HIPROC, 0x7fffffff)

ELF_dynamic_section_tags(ELF_expand_as_uint32_constant)
char const *
string_of_dynamic_section_tag(uint64_t const val) {
    switch (val) {
    ELF_dynamic_section_tags(ELF_expand_as_case)
    default: return "<unknown dynamic section tag>";
    }
}

#undef ELF_expand_as_uint32_constant
#undef ELF_expand_as_case

char const *
parse_header(io::read_t const &read, header64_or_32_t &dst) {
    ident_t ident;
    auto const bytes = io::read(read, ident.raw, EI_NIDENT);
    bool is_32 = false;
    if (bytes < EI_NIDENT) return "elf::parse_header: Ran out of bytes while reading e_ident.";
    if (memcmp(ident.f.magic_bytes, "\x7f""ELF", 4) != 0) return "elf::parse_header: magic bytes mismatch; expected 0x7f ELF.";
    switch (ident.f.elfclass) {
    case ELFCLASS32: is_32 = true; break;
    case ELFCLASS64: is_32 = false; break;
    default: return "elf::parse_header: EI_CLASS is neither ELFCLASS32 nor ELFCLASS64.";
    }
    switch (ident.f.elfdataformat) {
    case ELFDATA2LSB: break;
    default: return "elf::parse_header: EI_DATA is not ELFDATA2LSB.";
    }
    if (ident.f.elfversion != EV_CURRENT) return "elf::parse_header: EI_VERSION is not EV_CURRENT.";
    if (ident.f.osabi != ELFOSABI_NONE) return "elf::parse_header: EI_OSABI is not ELFOSABI_NONE.";
    dst.is_32 = is_32;
    memcpy(&dst.header64.e_ident, &ident, EI_NIDENT);
    if (is_32) {
        auto const remaining_size = sizeof(header_t<32>) - offsetof(header_t<32>, e_type);
        auto const bytes = io::read(read, &dst.header32.e_type, remaining_size);
        if (bytes < remaining_size) return "elf::parse_header: Ran out of bytes while reading the rest of the 32-bit ELF header.";
    } else {
        auto const remaining_size = sizeof(header_t<64>) - offsetof(header_t<64>, e_type);
        auto const bytes = io::read(read, &dst.header64.e_type, remaining_size);
        if (bytes < remaining_size) return "elf::parse_header: Ran out of bytes while reading the rest of the 64-bit ELF header.";
    }
    return nullptr;
}

template <size_t n>
char const *
parse_section_header(io::read_t const &read, header_t<n> const &hdr, section_header_t<n> &dst) {
    auto const total_bytes = hdr.e_shentsize;
    auto const required_bytes = sizeof(section_header_t<n>);
    auto const skip_bytes = total_bytes - required_bytes;
    if (required_bytes > total_bytes) return "elf::parse_section_header: section header is larger than e_shentsize.";
    auto const bytes = io::read(read, &dst, required_bytes);
    if (bytes < required_bytes) return "elf::parse_section_header: Ran out of bytes while reading a section header.";
    auto const success = io::seek(read, skip_bytes, io::Relative);
    if (!success) return "elf::parse_section_header: Ran out of bytes while skipping to the end of a section header.";
    return nullptr;
}

template <size_t n>
char const *
parse_dynamic_section_entry(io::read_t const &read, dynamic_section_entry_t<n> &dst) {
    auto const required_bytes = sizeof(dynamic_section_entry_t<n>);
    auto const bytes = io::read(read, &dst, required_bytes);
    if (bytes < required_bytes) return "elf::parse_dynamic_section_entry: Ran out of bytes while reading a dynamic section entry.";
    return nullptr;
}

} // namespace elf

#include <inttypes.h> // PRIu64
#include <stdarg.h> // va_list, va_start, va_end
#include <stdio.h> // vprintf, printf, fopen, fread, fwrite, fseeko, ftello
#include <stdlib.h> // exit

#if defined(STRIP_VERSIONED_SYMBOLS_verbose)
    #define dbgprint(...) printf(__VA_ARGS__)
#else
    #define dbgprint(...)
#endif

static void
failwith(char const * const fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    exit(1);
}

static size_t
cstdlib_write(void const * const dst, size_t const sz, void * const env) {
    auto const cstdlibfile = static_cast<FILE *>(env);
    return fwrite(dst, 1, sz, cstdlibfile);
}

static size_t
cstdlib_read(void * const dst, size_t const sz, void * const env) {
    auto const cstdlibfile = static_cast<FILE *>(env);
    return fread(dst, 1, sz, cstdlibfile);
}

static bool
cstdlib_seek(int64_t const offset, io::seek_origin_t const origin, void * const env) {
    auto const cstdlibfile = static_cast<FILE *>(env);
    int whence = SEEK_SET;
    switch (origin) {
    case io::Absolute: whence = SEEK_SET; break;
    case io::Relative: whence = SEEK_CUR; break;
    case io::End: whence = SEEK_END; break;
    }
    auto const ret = fseeko(cstdlibfile, offset, whence);
    return ret == 0;
}

static int64_t
cstdlib_curpos(void * const env) {
    auto const cstdlibfile = static_cast<FILE *>(env);
    return ftello(cstdlibfile);
}

static bool
cstdlib_flush(void * const env) {
    auto const cstdlibfile = static_cast<FILE *>(env);
    return fflush(cstdlibfile) == 0;
}

template <size_t n>
void
run(io::readwrite_t const &rw, elf::header_t<n> const &hdr) {
    auto const count = hdr.e_shnum;
    dbgprint("e_shnum = %d\n", count);
    if (!io::seek(rw, hdr.e_shoff, io::Absolute)) {
        failwith("Failed to seek to offset %" PRIu64 "; this is e_shoff in the ELF header.\n", static_cast<uint64_t>(hdr.e_shoff));
    }
    elf::section_header_t<n> dynamic;
    bool found_dynamic = false;
    for (uint16_t i = 0; i < count; ++i) {
        elf::section_header_t<n> shdr;
        auto const errstr = elf::parse_section_header(rw, hdr, shdr);
        if (errstr) {
            failwith("Couldn't parse elf section header. Reason: %s\n", errstr);
        }
        dbgprint("Found section:\n");
        dbgprint("    type = %s\n", elf::string_of_elf_section_header_type(shdr.sh_type));
        if (shdr.sh_type == elf::SHT_DYNAMIC) {
            dynamic = shdr;
            found_dynamic = true;
            break;
        }
    }
    if (!found_dynamic) {
        failwith("No dynamic section found.\n");
    }
    if (!io::seek(rw, dynamic.sh_offset, io::Absolute)) {
        failwith("Failed to seek to offset %" PRIu64 "; this is sh_offset in the SHT_DYNAMIC section header entry.\n", static_cast<uint64_t>(dynamic.sh_offset));
    }

    elf::dynamic_section_entry_t<n> entry;
    elf::dynamic_section_entry_t<n> *entries = nullptr;
    do {
        auto const errstr = elf::parse_dynamic_section_entry(rw, entry);
        if (errstr) {
            failwith("Failed to parse a dynamic section entry. Reason: %s\n", errstr);
        }
        dbgprint("Dynamic section entry found; tag = %s.\n", elf::string_of_dynamic_section_tag(entry.d_tag));
        arrput(entries, entry);
    } while (entry.d_tag != elf::DT_NULL);

    auto const num_entries = arrlen(entries);
    auto const begin = entries;
    auto const end = entries + num_entries;
    auto it = std::remove_if(begin, end, [] (elf::dynamic_section_entry_t<n> const &e) {
        switch (e.d_tag) {
        case elf::DT_VERSYM: return true;
        case elf::DT_VERNEED: return true;
        case elf::DT_VERNEEDNUM: return true;
        }
        return false;
    });
    if (it != end) {
        for (; it != end; ++it) {
            *it = elf::dynamic_section_entry_t<n> { };
        }

        if (!io::seek(rw, dynamic.sh_offset, io::Absolute)) {
            failwith("While preparing to write: failed to seek to offset %" PRIu64 "; this is sh_offset in the SHT_DYNAMIC section header entry.\n", static_cast<uint64_t>(dynamic.sh_offset));
        }
        auto const bytes_to_write = sizeof(elf::dynamic_section_entry_t<n>) * num_entries;
        auto const written = io::write(rw, entries, bytes_to_write);
        if (written < bytes_to_write) {
            failwith("Failure while writing updated .dynamic table.\n");
        }
        if (!io::flush(rw)) {
            failwith("Failure while flushing I/O output buffers.\n");
        }
    }
}

int
main(int argc, char **argv) {
    if (argc < 2) {
        failwith("The first argument should be a path to an elf executable.\n");
    }
    auto const path = argv[1];
    auto const cstdlibfile = fopen(path, "r+b");
    if (cstdlibfile == nullptr) {
        failwith("Couldn't open file: %s\n", path);
    }
    auto const rw = io::readwrite_t { { cstdlibfile, &cstdlib_read, &cstdlib_seek, &cstdlib_curpos }, &cstdlib_write, &cstdlib_flush };

    elf::header64_or_32_t elf_header;
    auto const errstr = elf::parse_header(rw, elf_header);
    if (errstr) {
        failwith("Couldn't parse elf header. Reason: %s\n", errstr);
    }
    dbgprint("Found %s-bit ELF executable.\n", elf_header.is_32 ? "32" : "64");
    if (elf_header.is_32) {
        run(rw, elf_header.header32);
    } else {
        run(rw, elf_header.header64);
    }

    return 0;
}
