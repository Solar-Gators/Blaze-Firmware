#include "fatfs_sd.h"

#ifdef SG_FATFS_SD_ENABLED

#include <string.h>

// SPI prescaler used after the card is initialized
#ifndef SD_SPI_FAST_PRESCALER
#define SD_SPI_FAST_PRESCALER SPI_BAUDRATEPRESCALER_2
#endif

namespace
{
constexpr BYTE CMD0 = 0;            // GO_IDLE_STATE
constexpr BYTE CMD1 = 1;            // SEND_OP_COND (MMC)
constexpr BYTE ACMD41 = 0x80 | 41;  // SEND_OP_COND (SDC)
constexpr BYTE CMD8 = 8;            // SEND_IF_COND
constexpr BYTE CMD9 = 9;            // SEND_CSD
constexpr BYTE CMD12 = 12;          // STOP_TRANSMISSION
constexpr BYTE CMD16 = 16;          // SET_BLOCKLEN
constexpr BYTE CMD17 = 17;          // READ_SINGLE_BLOCK
constexpr BYTE CMD18 = 18;          // READ_MULTIPLE_BLOCK
constexpr BYTE ACMD23 = 0x80 | 23;  // SET_WR_BLK_ERASE_COUNT (SDC)
constexpr BYTE CMD24 = 24;          // WRITE_BLOCK
constexpr BYTE CMD25 = 25;          // WRITE_MULTIPLE_BLOCK
constexpr BYTE CMD55 = 55;          // APP_CMD
constexpr BYTE CMD58 = 58;          // READ_OCR

// Card type flags
constexpr BYTE CT_MMC = 0x01;
constexpr BYTE CT_SD1 = 0x02;
constexpr BYTE CT_SD2 = 0x04;
constexpr BYTE CT_SDC = CT_SD1 | CT_SD2;
constexpr BYTE CT_BLOCK = 0x08;

// Data tokens
constexpr BYTE TOKEN_START_BLOCK = 0xFE;
constexpr BYTE TOKEN_START_MULTI_WRITE = 0xFC;
constexpr BYTE TOKEN_STOP_MULTI_WRITE = 0xFD;

constexpr UINT SECTOR_SIZE = 512;
constexpr uint32_t SPI_TIMEOUT_MS = 100;
constexpr uint32_t READY_TIMEOUT_MS = 500;
constexpr uint32_t DATA_TIMEOUT_MS = 200;
constexpr uint32_t INIT_TIMEOUT_MS = 1000;

SPI_HandleTypeDef *s_hspi = nullptr;
GPIO_TypeDef *s_csPort = nullptr;
uint16_t s_csPin = 0;
uint32_t s_slowPrescaler = SPI_BAUDRATEPRESCALER_256;
DSTATUS s_stat = STA_NOINIT;
BYTE s_cardType = 0;

void set_spi_prescaler(uint32_t prescaler)
{
    __HAL_SPI_DISABLE(s_hspi);
    MODIFY_REG(s_hspi->Instance->CR1, SPI_CR1_BR, prescaler);
    __HAL_SPI_ENABLE(s_hspi);
}

BYTE spi_xchg(BYTE tx)
{
    BYTE rx = 0xFF;
    if (HAL_SPI_TransmitReceive(s_hspi, &tx, &rx, 1, SPI_TIMEOUT_MS) != HAL_OK)
    {
        return 0xFF;
    }
    return rx;
}

bool spi_rx_multi(BYTE *buf, UINT len)
{
    memset(buf, 0xFF, len);
    return HAL_SPI_TransmitReceive(s_hspi, buf, buf, static_cast<uint16_t>(len), SPI_TIMEOUT_MS) ==
           HAL_OK;
}

bool spi_tx_multi(const BYTE *buf, UINT len)
{
    return HAL_SPI_Transmit(
               s_hspi, const_cast<uint8_t *>(buf), static_cast<uint16_t>(len), SPI_TIMEOUT_MS) ==
           HAL_OK;
}

void cs_low()
{
    HAL_GPIO_WritePin(s_csPort, s_csPin, GPIO_PIN_RESET);
}
void cs_high()
{
    HAL_GPIO_WritePin(s_csPort, s_csPin, GPIO_PIN_SET);
}

bool wait_ready(uint32_t timeoutMs)
{
    const uint32_t start = HAL_GetTick();
    do
    {
        if (spi_xchg(0xFF) == 0xFF)
        {
            return true;
        }
    } while ((HAL_GetTick() - start) < timeoutMs);
    return false;
}

void deselect()
{
    cs_high();
    spi_xchg(0xFF);
}

bool select()
{
    cs_low();
    spi_xchg(0xFF);
    if (wait_ready(READY_TIMEOUT_MS))
    {
        return true;
    }
    deselect();
    return false;
}

BYTE send_cmd(BYTE cmd, DWORD arg)
{
    if (cmd & 0x80)
    {
        cmd &= 0x7F;
        const BYTE res = send_cmd(CMD55, 0);
        if (res > 1)
        {
            return res;
        }
    }

    if (cmd != CMD12)
    {
        deselect();
        if (!select())
        {
            return 0xFF;
        }
    }

    BYTE crc = 0x01;
    if (cmd == CMD0)
        crc = 0x95;
    if (cmd == CMD8)
        crc = 0x87;

    const BYTE frame[6] = {
        static_cast<BYTE>(0x40 | cmd),
        static_cast<BYTE>(arg >> 24),
        static_cast<BYTE>(arg >> 16),
        static_cast<BYTE>(arg >> 8),
        static_cast<BYTE>(arg),
        crc,
    };
    if (!spi_tx_multi(frame, sizeof(frame)))
    {
        return 0xFF;
    }

    if (cmd == CMD12)
    {
        spi_xchg(0xFF);
    }

    BYTE res;
    int tries = 10;
    do
    {
        res = spi_xchg(0xFF);
    } while ((res & 0x80) && --tries);

    return res;
}

bool rcvr_datablock(BYTE *buf, UINT len)
{
    const uint32_t start = HAL_GetTick();
    BYTE token;
    do
    {
        token = spi_xchg(0xFF);
    } while (token == 0xFF && (HAL_GetTick() - start) < DATA_TIMEOUT_MS);

    if (token != TOKEN_START_BLOCK)
    {
        return false;
    }
    if (!spi_rx_multi(buf, len))
    {
        return false;
    }
    spi_xchg(0xFF);
    spi_xchg(0xFF);
    return true;
}

bool xmit_datablock(const BYTE *buf, BYTE token)
{
    if (!wait_ready(READY_TIMEOUT_MS))
    {
        return false;
    }

    spi_xchg(token);
    if (token != TOKEN_STOP_MULTI_WRITE)
    {
        if (!spi_tx_multi(buf, SECTOR_SIZE))
        {
            return false;
        }
        spi_xchg(0xFF);
        spi_xchg(0xFF);
        const BYTE resp = spi_xchg(0xFF);
        if ((resp & 0x1F) != 0x05)
        {
            return false;
        }
    }
    return true;
}

bool timed_out(uint32_t start)
{
    return (HAL_GetTick() - start) >= INIT_TIMEOUT_MS;
}

bool wait_card_ready(BYTE cmd, DWORD arg, uint32_t start)
{
    while (!timed_out(start))
    {
        if (send_cmd(cmd, arg) == 0)
        {
            return true;
        }
    }
    return false;
}
}  // namespace

void SD_disk_config(SPI_HandleTypeDef *hspi, GPIO_TypeDef *csPort, uint16_t csPin)
{
    s_hspi = hspi;
    s_csPort = csPort;
    s_csPin = csPin;
    s_slowPrescaler = hspi->Init.BaudRatePrescaler;
    s_stat = STA_NOINIT;
    s_cardType = 0;
    cs_high();
}

DSTATUS SD_disk_initialize(BYTE pdrv)
{
    if (pdrv != 0 || s_hspi == nullptr)
    {
        return STA_NOINIT;
    }

    set_spi_prescaler(s_slowPrescaler);

    cs_high();
    for (int i = 0; i < 10; i++)
    {
        spi_xchg(0xFF);
    }

    BYTE type = 0;

    BYTE r1 = 0xFF;
    for (int i = 0; i < 10 && r1 != 1; i++)
    {
        r1 = send_cmd(CMD0, 0);
    }

    if (r1 == 1)
    {
        const uint32_t start = HAL_GetTick();
        BYTE ocr[4];

        if (send_cmd(CMD8, 0x1AA) == 1)
        {
            if (spi_rx_multi(ocr, 4) && ocr[2] == 0x01 && ocr[3] == 0xAA)
            {
                if (wait_card_ready(ACMD41, 1UL << 30, start) && send_cmd(CMD58, 0) == 0 &&
                    spi_rx_multi(ocr, 4))
                {
                    type = (ocr[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
                }
            }
        }
        else
        {
            BYTE cmd;
            if (send_cmd(ACMD41, 0) <= 1)
            {
                type = CT_SD1;
                cmd = ACMD41;
            }
            else
            {
                type = CT_MMC;
                cmd = CMD1;
            }

            if (!wait_card_ready(cmd, 0, start) || send_cmd(CMD16, SECTOR_SIZE) != 0)
            {
                type = 0;
            }
        }
    }

    s_cardType = type;
    deselect();

    if (type)
    {
        s_stat = static_cast<DSTATUS>(s_stat & ~STA_NOINIT);
        set_spi_prescaler(SD_SPI_FAST_PRESCALER);
    }
    else
    {
        s_stat = static_cast<DSTATUS>(s_stat | STA_NOINIT);
    }

    return s_stat;
}

DSTATUS SD_disk_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return s_stat;
}

DRESULT SD_disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || buff == nullptr || count == 0)
    {
        return RES_PARERR;
    }
    if (s_stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    if (!(s_cardType & CT_BLOCK))
    {
        sector *= SECTOR_SIZE;
    }

    if (count == 1)
    {
        if (send_cmd(CMD17, sector) == 0 && rcvr_datablock(buff, SECTOR_SIZE))
        {
            count = 0;
        }
    }
    else
    {
        if (send_cmd(CMD18, sector) == 0)
        {
            do
            {
                if (!rcvr_datablock(buff, SECTOR_SIZE))
                {
                    break;
                }
                buff += SECTOR_SIZE;
            } while (--count);
            send_cmd(CMD12, 0);
        }
    }

    deselect();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || buff == nullptr || count == 0)
    {
        return RES_PARERR;
    }
    if (s_stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }
    if (s_stat & STA_PROTECT)
    {
        return RES_WRPRT;
    }
    if (!(s_cardType & CT_BLOCK))
    {
        sector *= SECTOR_SIZE;
    }
    if (count == 1)
    {
        if (send_cmd(CMD24, sector) == 0 && xmit_datablock(buff, TOKEN_START_BLOCK))
        {
            count = 0;
        }
    }
    else
    {
        if (s_cardType & CT_SDC)
        {
            send_cmd(ACMD23, count);
        }
        if (send_cmd(CMD25, sector) == 0)
        {
            do
            {
                if (!xmit_datablock(buff, TOKEN_START_MULTI_WRITE))
                {
                    break;
                }
                buff += SECTOR_SIZE;
            } while (--count);

            if (!xmit_datablock(nullptr, TOKEN_STOP_MULTI_WRITE))
            {
                count = 1;
            }
        }
    }

    deselect();
    return count ? RES_ERROR : RES_OK;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }
    if (s_stat & STA_NOINIT)
    {
        return RES_NOTRDY;
    }

    DRESULT res = RES_ERROR;

    switch (cmd)
    {
        case CTRL_SYNC:
            if (select())
            {
                res = RES_OK;
            }
            break;

        case GET_SECTOR_COUNT:
        {
            BYTE csd[16];
            if (buff != nullptr && send_cmd(CMD9, 0) == 0 && rcvr_datablock(csd, sizeof(csd)))
            {
                DWORD sectors;
                if ((csd[0] >> 6) == 1)
                {
                    const DWORD csize = csd[9] | (static_cast<DWORD>(csd[8]) << 8) |
                                        (static_cast<DWORD>(csd[7] & 0x3F) << 16);
                    sectors = (csize + 1) << 10;
                }
                else
                {
                    const BYTE shift =
                        (csd[5] & 0x0F) + ((csd[10] & 0x80) >> 7) + ((csd[9] & 0x03) << 1) + 2;
                    const DWORD csize = (csd[8] >> 6) + (static_cast<DWORD>(csd[7]) << 2) +
                                        (static_cast<DWORD>(csd[6] & 0x03) << 10) + 1;
                    sectors = csize << (shift - 9);
                }
                *static_cast<DWORD *>(buff) = sectors;
                res = RES_OK;
            }
            break;
        }

        case GET_SECTOR_SIZE:
            if (buff != nullptr)
            {
                *static_cast<WORD *>(buff) = SECTOR_SIZE;
                res = RES_OK;
            }
            break;

        case GET_BLOCK_SIZE:
            if (buff != nullptr)
            {
                *static_cast<DWORD *>(buff) = 1;
                res = RES_OK;
            }
            break;

        default:
            res = RES_PARERR;
            break;
    }

    deselect();
    return res;
}

#endif  // SG_FATFS_SD_ENABLED
