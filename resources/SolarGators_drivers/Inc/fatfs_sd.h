//
// Created by lucas on 11/2/2026.
//
// SD card (SPI mode) disk I/O driver for FatFs.
// Call SD_disk_config() once before mounting, then route the FatFs
// diskio callbacks (USER_initialize, USER_read, ...) to the SD_disk_* functions.
//
// Only compiled on boards that have both FatFs (diskio.h) and the HAL SPI module.
//

#ifndef BLAZEFIRMWARE_FATFS_SD_H
#define BLAZEFIRMWARE_FATFS_SD_H

#include "main.h"

#if defined(HAL_SPI_MODULE_ENABLED) && defined(__has_include)
#if __has_include("diskio.h")
#define SG_FATFS_SD_ENABLED 1
#endif
#endif

#ifdef SG_FATFS_SD_ENABLED

#include "diskio.h"

#ifdef __cplusplus
extern "C"
{
#endif

  void SD_disk_config(SPI_HandleTypeDef *hspi, GPIO_TypeDef *csPort, uint16_t csPin);

  DSTATUS SD_disk_initialize(BYTE pdrv);
  DSTATUS SD_disk_status(BYTE pdrv);
  DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
  DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
  DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff);

#ifdef __cplusplus
}
#endif

#endif  // SG_FATFS_SD_ENABLED
#endif  // BLAZEFIRMWARE_FATFS_SD_H
