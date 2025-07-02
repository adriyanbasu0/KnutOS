#ifndef FADT_H
#define FADT_H

#include <stdint.h>
#include <firmware/acpi/tables/sdth.h>

// FADT structure (ACPI 2.0+)
typedef struct {
    sdt_header_t header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferred_pm_profile;
    uint16_t sci_int;
    uint32_t smi_cmd;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_evt_blk;
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;
    uint32_t pm1b_cnt_blk;
    // ... (other fields omitted for brevity)
    uint16_t pm1_evt_len;
    uint16_t pm1_cnt_len;
    // ... (add more fields as needed)
} __attribute__((packed)) fadt_t;

#endif // FADT_H
