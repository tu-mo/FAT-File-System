/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * Includes
 ******************************************************************************/
#define KMC_DEFAULT_SECTOR_SIZE 512U

/*******************************************************************************
 * Global Variables
 ******************************************************************************/
static FILE *sp_disk = NULL;
static uint16_t s_byte_per_sector = 0;

/*******************************************************************************
 * Codes
 ******************************************************************************/
/* Function is used to initialize HAL */
bool kmc_init(const uint8_t *const file_path)
{
    bool retVal = true;

    sp_disk = fopen(file_path, "rb");
    if (sp_disk != NULL)
    {
        s_byte_per_sector = KMC_DEFAULT_SECTOR_SIZE;
    }
    else
    {
        retVal = false;
    }

    return retVal;
}

/* Function is used to update size of sector */
void kmc_update_sector_size(const uint16_t size)
{
    s_byte_per_sector = size;
}

/* Function is used to read sector */
int32_t kmc_read_sector(uint32_t index, uint8_t *p_buff)
{
    int32_t retVal = 0;

    if ((sp_disk != NULL) && (p_buff != NULL))
    {
        if (index != 0)
        {
            fseek(sp_disk, KMC_DEFAULT_SECTOR_SIZE +
                  s_byte_per_sector * (index - 1),
                  SEEK_SET);
        }
        else
        {
            /* Do nothing */
        }
        retVal = fread(p_buff, sizeof(uint8_t), s_byte_per_sector, sp_disk);
    }
    else
    {
        /* Do nothing */
    }

    return retVal;
}

/* Function is used to read multi sector */
int32_t kmc_read_multi_sector(uint32_t index, uint32_t num,
                              uint8_t *p_buff)
{
    int32_t retVal = 0;
    uint8_t i = 0;
    uint16_t temp = 0;

    if ((sp_disk != NULL) && (p_buff != NULL))
    {
        for (i = 0; i < num; i++)
        {
            temp = kmc_read_sector(index + i, p_buff + i * s_byte_per_sector);
            retVal += (uint32_t)temp;
        }
    }
    else
    {
        /* Do nothing */
    }

    return retVal;
}

/* Function is used to de-initialize HAL */
void kmc_deinit(void)
{
    fclose(sp_disk);
    sp_disk = NULL;
    s_byte_per_sector = 0;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
