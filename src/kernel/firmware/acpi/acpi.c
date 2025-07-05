/*
	This file is part of an x86_64 hobbyist operating system called KnutOS
	Everything is openly developed on GitHub: https://github.com/Tix3Dev/KnutOS/

	Copyright (C) 2021-2022  Yves Vollmeier <https://github.com/Tix3Dev>
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stddef.h>
#include <stdint.h>

#include <boot/stivale2.h>
#include <boot/stivale2_boot.h>
#include <firmware/acpi/tables/fadt.h>
#include <firmware/acpi/tables/madt.h>
#include <firmware/acpi/tables/rsdp.h>
#include <firmware/acpi/tables/rsdt.h>
#include <firmware/acpi/tables/sdth.h>
#include <firmware/acpi/acpi.h>
#include <memory/mem.h>
#include <libk/debug/debug.h>
#include <libk/io/io.h>
#include <libk/log/log.h>
#include <libk/stdio/stdio.h>
#include <libk/string/string.h>

static rsdt_structure_t *rsdt;

// TODO: write description when fully done (at least when MADT and APIC are done)
void acpi_init(struct stivale2_struct *stivale2_struct)
{
    struct stivale2_struct_tag_rsdp *rsdp_tag = stivale2_get_tag(stivale2_struct,
            STIVALE2_STRUCT_TAG_RSDP_ID);

    rsdp_init(rsdp_tag->rsdp);

    rsdt = (rsdt_structure_t *)phys_to_higher_half_data((uintptr_t)get_rsdp_structure()->rsdt_address);

    // having a RSDT is equivalent to having ACPI supported
    if (acpi_check_sdt_header(&rsdt->header, "RSDT") != 0)
    {
        serial_log(ERROR, "No ACPI was found on this computer!\n");
        kernel_log(ERROR, "No ACPI was found on this computer!\n");


        serial_log(ERROR, "Kernel halted!\n");
        kernel_log(ERROR, "Kernel halted!\n");

        for (;;)
            asm ("hlt");
    }

    madt_init();

    serial_log(INFO, "ACPI initialized\n");
    kernel_log(INFO, "ACPI initialized\n");
}

// check if the signature of the SDT header matches the parameter (signature) and
// verify it's checksum
int acpi_check_sdt_header(sdt_header_t *sdt_header, const char *signature)
{
    if (memcmp(sdt_header->signature, signature, 4) == 0 &&
            acpi_verify_sdt_header_checksum(sdt_header, signature) == 0)
        return 0;

    return 1;
}

// check if the sum of all bytes in a SDT header is equal to zero
int acpi_verify_sdt_header_checksum(sdt_header_t *sdt_header, const char *signature)
{
    uint8_t checksum = 0;
    uint8_t *ptr = (uint8_t *)sdt_header;
    uint8_t current_byte;

    serial_log(INFO, "Verifying %s checksum:\n", signature);
    kernel_log(INFO, "Verifying %s checksum:\n", signature);

    serial_set_color(TERM_PURPLE);

    debug("First %d bytes are being checked: ", sdt_header->length);
    printk(GFX_PURPLE, "First %d bytes are being checked: ", sdt_header->length);

    for (uint8_t i = 0; i < sdt_header->length; i++)
    {
        current_byte = ptr[i];
        debug("%x ", current_byte);
        printk(GFX_PURPLE, "%x ", current_byte);

        checksum += current_byte;
    }

    debug("\n");
    printk(GFX_PURPLE, "\n");

    serial_set_color(TERM_COLOR_RESET);

    checksum = checksum & 0xFF;

    if (checksum == 0)
    {
        serial_log(INFO, "%s checksum is verified.\n", signature);
        kernel_log(INFO, "%s checksum is verified.\n", signature);

        return 0;
    }
    else
    {
        serial_log(ERROR, "%s checksum isn't 0! Checksum: 0x%x\n", signature, checksum);
        kernel_log(ERROR, "%s checksum isn't 0! Checksum: 0x%x\n", signature, checksum);

        return 1;
    }
}

// traverse RSDT to find table according to identifier
sdt_header_t *acpi_find_sdt_table(const char *signature)
{
    size_t entry_count = (rsdt->header.length - sizeof(rsdt->header)) / (has_xsdt() ? 8 : 4);
    sdt_header_t *current_entry;

    for (size_t i = 0; i < entry_count; i++)
    {
        current_entry = (sdt_header_t *)(uintptr_t)rsdt->entries[i];

        if (acpi_check_sdt_header(current_entry, signature) == 0)
            return (sdt_header_t *)phys_to_higher_half_data((uintptr_t)current_entry);
    }

    serial_log(ERROR, "Could not find SDT with signature '%s'!\n", signature);
    kernel_log(ERROR, "Could not find SDT with signature '%s'!\n", signature);

    return NULL;
}

// Helper: Find S5 sleep type in DSDT AML (robust AML search)
static int acpi_find_s5(uint8_t *dsdt, size_t dsdt_len, uint8_t *slp_typ) {
    // Search for both AML and ASCII _S5_ patterns
    for (size_t i = 0; i < dsdt_len - 8; i++) {
        // AML NameOp + _S5_ (0x08 0x5F 0x53 0x35 0x5F)
        if (dsdt[i] == 0x08 && dsdt[i+1] == 0x5F && dsdt[i+2] == 0x53 && dsdt[i+3] == 0x35 && dsdt[i+4] == 0x5F) {
            for (size_t j = i + 5; j < i + 32 && j < dsdt_len; j++) {
                if (dsdt[j] == 0x12) { // PackageOp
                    size_t k = j+3;
                    if (dsdt[k] == 0x0A) k++;
                    *slp_typ = dsdt[k];
                    return 0;
                }
            }
        }
        // ASCII _S5_ (0x5F 0x53 0x35 0x5F)
        if (dsdt[i] == 0x5F && dsdt[i+1] == 0x53 && dsdt[i+2] == 0x35 && dsdt[i+3] == 0x5F) {
            // Try to find PackageOp (0x12) after this
            for (size_t j = i + 4; j < i + 32 && j < dsdt_len; j++) {
                if (dsdt[j] == 0x12) {
                    size_t k = j+3;
                    if (dsdt[k] == 0x0A) k++;
                    *slp_typ = dsdt[k];
                    return 0;
                }
            }
        }
    }
    // Fallback: use SLP_TYP = 5 (common for QEMU/Bochs/VirtualBox)
    *slp_typ = 5;
    serial_log(ERROR, "_S5_ not found, using fallback SLP_TYP=5 (may not work on real hardware)\n");
    return 0;
}

// ACPI shutdown function
#include <libk/io/io.h>
#include <stddef.h>
#include <stdint.h>

struct facp {
    char     signature[4];
    uint32_t length;
    uint8_t  unneeded1[40 - 8];
    uint32_t dsdt;
    uint8_t  unneeded2[48 - 44];
    uint32_t SMI_CMD;
    uint8_t  ACPI_ENABLE;
    uint8_t  ACPI_DISABLE;
    uint8_t  unneeded3[64 - 54];
    uint32_t PM1a_CNT_BLK;
    uint32_t PM1b_CNT_BLK;
    uint8_t  unneeded4[89 - 72];
    uint8_t  PM1_CNT_LEN;
};

static inline uint8_t parse_integer(uint8_t* s5_addr, uint64_t* value) {
    uint8_t op = *s5_addr++;
    if (op == 0x0) { *value = 0; return 1; }
    else if (op == 0x1) { *value = 1; return 1; }
    else if (op == 0xFF) { *value = ~0; return 1; }
    else if (op == 0xA) { *value = s5_addr[0]; return 2; }
    else if (op == 0xB) { *value = s5_addr[0] | ((uint16_t)s5_addr[1] << 8); return 3; }
    else if (op == 0xC) { *value = s5_addr[0] | ((uint32_t)s5_addr[1] << 8) | ((uint32_t)s5_addr[2] << 16) | ((uint32_t)s5_addr[3] << 24); return 5; }
    else if (op == 0xE) { *value = s5_addr[0] | ((uint64_t)s5_addr[1] << 8) | ((uint64_t)s5_addr[2] << 16) | ((uint64_t)s5_addr[3] << 24) | ((uint64_t)s5_addr[4] << 32) | ((uint64_t)s5_addr[5] << 40) | ((uint64_t)s5_addr[6] << 48) | ((uint64_t)s5_addr[7] << 56); return 9; }
    else { return 0; }
}

static void *find_sdt_adapter(const char *sig, size_t idx) {
    return acpi_find_sdt_table(sig);
}

int acpi_shutdown_hack(
        uintptr_t direct_map_base,
        void     *(*find_sdt)(const char *signature, size_t index),
        uint8_t   (*inb)(uint16_t port),
        uint16_t  (*inw)(uint16_t port),
        void      (*outb)(uint16_t port, uint8_t value),
        void      (*outw)(uint16_t port, uint16_t value)
    ) {
    struct facp *facp = find_sdt("FACP", 0);
    if (!facp) return -1;
    uint8_t *dsdt_ptr = (uint8_t *)(uintptr_t)facp->dsdt + 36 + direct_map_base;
    size_t   dsdt_len = *((uint32_t *)((uintptr_t)facp->dsdt + 4 + direct_map_base)) - 36;
    uint8_t *s5_addr = 0;
    for (size_t i = 0; i < dsdt_len; i++) {
        if ((dsdt_ptr + i)[0] == '_' && (dsdt_ptr + i)[1] == 'S' && (dsdt_ptr + i)[2] == '5' && (dsdt_ptr + i)[3] == '_') {
            s5_addr = dsdt_ptr + i;
            goto s5_found;
        }
    }
    return -1;
s5_found:
    s5_addr += 4;
    if (*s5_addr++ != 0x12) return -1;
    s5_addr += ((*s5_addr & 0xc0) >> 6) + 1;
    if (*s5_addr++ < 2) return -1;
    uint64_t value = 0;
    uint8_t size = parse_integer(s5_addr, &value);
    if (size == 0) return -1;
    uint16_t SLP_TYPa = value << 10;
    s5_addr += size;
    size = parse_integer(s5_addr, &value);
    if (size == 0) return -1;
    uint16_t SLP_TYPb = value << 10;
    s5_addr += size;
    if(facp->SMI_CMD != 0 && facp->ACPI_ENABLE != 0) {
        outb(facp->SMI_CMD, facp->ACPI_ENABLE);
        for (int i = 0; i < 100; i++) inb(0x80);
        while (!(inw(facp->PM1a_CNT_BLK) & (1 << 0)));
    }
    outw(facp->PM1a_CNT_BLK, SLP_TYPa | (1 << 13));
    if (facp->PM1b_CNT_BLK)
        outw(facp->PM1b_CNT_BLK, SLP_TYPb | (1 << 13));
    for (int i = 0; i < 100; i++) inb(0x80);
    return -1;
}

void acpi_shutdown(void) {
    int result = acpi_shutdown_hack(
        0, // direct_map_base (adjust if needed)
        find_sdt_adapter,
        io_inb,
        io_inw,
        io_outb,
        io_outw
    );
    if (result != 0) {
        serial_log(ERROR, "ACPI shutdown failed: FACP/DSDT/PM1 control blocks may be missing or ACPI not enabled.\n");
        kernel_log(ERROR, "ACPI shutdown failed: FACP/DSDT/PM1 control blocks may be missing or ACPI not enabled.\n");
        serial_log(ERROR, "This can happen after a soft reboot. Try a cold boot for shutdown to work.\n");
        kernel_log(ERROR, "This can happen after a soft reboot. Try a cold boot for shutdown to work.\n");
    }
    for (;;) asm volatile ("hlt");
}
