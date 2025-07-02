#include <firmware/acpi/tables/fadt.h>
#include <stddef.h>

// Helper to get FADT from SDT header pointer
fadt_t *fadt_from_sdt(sdt_header_t *sdt) {
    if (!sdt) return NULL;
    return (fadt_t *)sdt;
}
