#ifndef WINTAKOS_AHCI_H
#define WINTAKOS_AHCI_H

#include "../include/types.h"
#include "ata.h"

bool          ahci_init(void);
bool          ahci_is_present(void);
ata_drive_t*  ahci_get_info(void);
bool          ahci_read_sectors(uint32_t lba, uint8_t count, void* buf);
bool          ahci_write_sectors(uint32_t lba, uint8_t count, const void* buf);

#endif
