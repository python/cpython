/* Bounded section lookup in a copy of a remote Mach-O header and commands. */
#ifndef Py_REMOTE_DEBUG_MACHO_H
#define Py_REMOTE_DEBUG_MACHO_H

#include <mach-o/loader.h>
#include "internal/pycore_bitutils.h"

static uint32_t
macho_u32(uint32_t value, int swap)
{
    return swap ? _Py_bswap32(value) : value;
}

static uint64_t
macho_u64(uint64_t value, int swap)
{
    return swap ? _Py_bswap64(value) : value;
}

static uintptr_t
macho_invalid_image(void)
{
    PyErr_SetString(PyExc_RuntimeError, "Invalid or truncated Mach-O image");
    return 0;
}

static size_t
macho_header_size(uint32_t magic)
{
    switch (magic) {
        case MH_MAGIC:
        case MH_CIGAM:
            return sizeof(struct mach_header);
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            return sizeof(struct mach_header_64);
        default:
            return 0;
    }
}

static uintptr_t
macho_section_address(const void *image, size_t image_size,
                      uintptr_t base, const char *name)
{
    if (strlen(name) > 16) {
        return 0;
    }
    const unsigned char *bytes = image;
    uint32_t magic;
    if (image_size < sizeof(magic)) {
        return macho_invalid_image();
    }
    memcpy(&magic, bytes, sizeof(magic));

    int wide = magic == MH_MAGIC_64 || magic == MH_CIGAM_64;
    int swap = magic == MH_CIGAM || magic == MH_CIGAM_64;
    size_t header_size = macho_header_size(magic);
    if (header_size == 0 || image_size < header_size) {
        return macho_invalid_image();
    }
    struct mach_header header;
    memcpy(&header, bytes, sizeof(header));
    size_t commands_size = macho_u32(header.sizeofcmds, swap);
    if (commands_size > image_size - header_size) {
        return macho_invalid_image();
    }

    const unsigned char *cursor = bytes + header_size;
    size_t remaining = commands_size;
    uint64_t header_address = 0, section_address = 0;
    int have_header = 0, have_section = 0;
    uint32_t commands = macho_u32(header.ncmds, swap);
    for (uint32_t index = 0; index < commands; index++) {
        struct load_command command;
        if (remaining < sizeof(command)) {
            return macho_invalid_image();
        }
        memcpy(&command, cursor, sizeof(command));
        uint32_t kind = macho_u32(command.cmd, swap);
        size_t length = macho_u32(command.cmdsize, swap);
        if (length < sizeof(command) || length > remaining) {
            return macho_invalid_image();
        }
        if (kind == (wide ? LC_SEGMENT_64 : LC_SEGMENT)) {
            uint64_t address, file_offset, file_size;
            uint32_t sections;
            size_t segment_size, section_size;
            int data_segment;
            if (wide) {
                struct segment_command_64 segment;
                segment_size = sizeof(segment);
                section_size = sizeof(struct section_64);
                if (length < segment_size) {
                    return macho_invalid_image();
                }
                memcpy(&segment, cursor, segment_size);
                address = macho_u64(segment.vmaddr, swap);
                file_offset = macho_u64(segment.fileoff, swap);
                file_size = macho_u64(segment.filesize, swap);
                sections = macho_u32(segment.nsects, swap);
                data_segment = strncmp(segment.segname, "__DATA", 16) == 0;
            }
            else {
                struct segment_command segment;
                segment_size = sizeof(segment);
                section_size = sizeof(struct section);
                if (length < segment_size) {
                    return macho_invalid_image();
                }
                memcpy(&segment, cursor, segment_size);
                address = macho_u32(segment.vmaddr, swap);
                file_offset = macho_u32(segment.fileoff, swap);
                file_size = macho_u32(segment.filesize, swap);
                sections = macho_u32(segment.nsects, swap);
                data_segment = strncmp(segment.segname, "__DATA", 16) == 0;
            }
            if (sections > (length - segment_size) / section_size) {
                return macho_invalid_image();
            }
            /* The file-backed segment at offset zero contains the header.
             * __PAGEZERO has no file contents and does not define the slide. */
            if (file_offset == 0 && file_size != 0) {
                header_address = address;
                have_header = 1;
            }
            if (data_segment) {
                const unsigned char *record = cursor + segment_size;
                for (uint32_t item = 0; item < sections;
                     item++, record += section_size)
                {
                    if (strncmp((const char *)record, name, 16) != 0) {
                        continue;
                    }
                    if (wide) {
                        struct section_64 section;
                        memcpy(&section, record, sizeof(section));
                        section_address = macho_u64(section.addr, swap);
                    }
                    else {
                        struct section section;
                        memcpy(&section, record, sizeof(section));
                        section_address = macho_u32(section.addr, swap);
                    }
                    have_section = 1;
                }
            }
        }
        cursor += length;
        remaining -= length;
    }
    if (!have_section) {
        return 0;
    }
    if (!have_header || section_address < header_address ||
        section_address - header_address > UINTPTR_MAX - base)
    {
        return macho_invalid_image();
    }
    return base + (uintptr_t)(section_address - header_address);
}

#endif /* Py_REMOTE_DEBUG_MACHO_H */
