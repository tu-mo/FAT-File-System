/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fat.h"
#include "hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* Boot sector */
#define FATFS_BOOT_SECTOR_SIZE 512U
#define FATFS_BOOT_SECTOR_INDEX 0U
#define FATFS_BYTE_PER_SECTOR_OFFSET 0x0BU
#define FATFS_BYTE_PER_SECTOR_BYTES 2U
#define FATFS_SECTOR_PER_CLUSTER_OFFSET 0x0DU
#define FATFS_SECTOR_PER_CLUSTER_BYTES 1U
#define FATFS_NUMBER_SECTOR_BEFORE_FAT_OFFSET 0x0EU
#define FATFS_NUMBER_SECTOR_BEFORE_FAT_BYTES 2U
#define FATFS_NUMBER_FAT_OFFSET 0x10U
#define FATFS_NUMBER_FAT_BYTES 1U
#define FATFS_NUMBER_ENTRY_RDET_OFFSET 0x11U
#define FATFS_NUMBER_ENTRY_RDET_BYTES 2U
#define FATFS_SECTOR_PER_FAT_OFFSET 0x16U
#define FATFS_SECTOR_PER_FAT_BYTES 2U
#define FATFS_ROOT_ENTRY_OFFSET 0x11U
#define FATFS_ROOT_ENTRY_BYTES 2U
#define FATFS_FAT_TYPE_OFFSET 0x36U
#define FATFS_FAT_TYPE_SIZE 8U
#define FATFS_ENTRY_SIZE 32U

/* Main entry */
#define FATFS_MAIN_ENTRY_FILE_NAME_OFFSET 0x00U
#define FATFS_MAIN_ENTRY_FILE_NAME_BYTES 8U
#define FATFS_MAIN_ENTRY_FILE_EXTENSION_OFFSET 0x08U
#define FATFS_MAIN_ENTRY_FILE_EXTENSION_BYTES 3U
#define FATFS_MAIN_ENTRY_ATTRIBUTE_OFFSET 0x0BU
#define FATFS_MAIN_ENTRY_ATTRIBUTE_BYTES 1U
#define FATFS_MAIN_ENTRY_FIRST_CLUSTER_OFFSET 0x1AU
#define FATFS_MAIN_ENTRY_FIRST_CLUSTER_BYTES 2U
#define FATFS_MAIN_ENTRY_FILE_SIZE_OFFSET 0x1CU
#define FATFS_MAIN_ENTRY_FILE_SIZE_BYTES 4U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_OFFSET 0x16U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_BYTES 2U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_SECOND_MASK 0x001FU
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_SECOND_SHIFT 0U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_MINUTE_MASK 0x07E0U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_MINUTE_SHIFT 5U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_HOUR_MASK 0xF800U
#define FATFS_MAIN_ENTRY_MODIFIED_TIME_HOUR_SHIFT 11U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_OFFSET 0x18U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_BYTES 2U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_DAY_MASK 0x001FU
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_DAY_SHIFT 0U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_MONTH_MASK 0x01E0U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_MONTH_SHIFT 5U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_YEAR_MASK 0xFE00U
#define FATFS_MAIN_ENTRY_MODIFIED_DATE_YEAR_SHIFT 9U
#define FATFS_FILE_ATTRIBUTE 0x00U
#define FATFS_SUBDIRECTORY_ATTRIBUTE 0x10U
#define FATFS_SUBENTRY_ATTRIBUTE 0x0FU

/* Sub entry */
#define FATFS_SUB_ENTRY_FIRST_FIVE_CHARACTER_OFFSET 0x01U
#define FATFS_SUB_ENTRY_FIRST_FIVE_CHARACTER_BYTES 10U
#define FATFS_SUB_ENTRY_NEXT_SIX_CHARACTER_OFFSET 0x0EU
#define FATFS_SUB_ENTRY_NEXT_SIX_CHARACTER_BYTES 12U
#define FATFS_SUB_ENTRY_NEXT_TWO_CHARACTER_OFFSET 0x1CU
#define FATFS_SUB_ENTRY_NEXT_TWO_CHARACTER_BYTES 4U
#define FATFS_SUB_ENTRY_DATA_BYTES 13U

#define make_value_little_endian(first_byte, second_byte) \
    ((second_byte << 8) | first_byte)
#define make_even_element_fat(first_index, second_index) \
    (((second_index & 0x0F) << 8) | first_index)
#define make_odd_element_fat(first_index, second_index) \
    ((second_index << 4) | ((first_index & 0xF0) >> 4))

typedef enum
{
    EMPTY_ENTRY,
    MAIN_ENTRY,
    SUB_ENTRY
} fatfs_entry_type_enum_t;

static fatfs_boot_sector_struct_t s_boot_info = {0, 0, 0, 0};
static fatfs_entry_info_struct_t *sp_entry_list_head = NULL;
static fatfs_entry_info_struct_t *sp_entry_list_tail = NULL;
static uint32_t s_end_cluster = 0;

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/**
 * @brief Decode entry
 *
 * @param [in] p_entry is entry to decode
 * @param [out] p_info is data of entry after decode
 * @param [out] p_buff is data of sub-entry
 * @return fatfs_entry_type_enum_t is type of entry
 */
static fatfs_entry_type_enum_t fatfs_decode_entry(const uint8_t *const
        p_entry , fatfs_entry_info_struct_t *const p_info,
        uint8_t *const p_buff);

/**
 * @brief Get next cluster
 *
 * @param [inout] next_cluster is next cluster
 * @return fatfs_error_enum_t is error code
 */
static fatfs_error_enum_t fatfs_get_next_cluster(uint32_t *const
        next_cluster);

/**
 * @brief Inset entry to list
 *
 * @param [inout] new_entry is entry to insert
 */
static void fatfs_insert(fatfs_entry_info_struct_t *const new_entry);

/*******************************************************************************
 * Codes
 ******************************************************************************/
/* Function is used to decode entry */
static fatfs_entry_type_enum_t fatfs_decode_entry(const uint8_t *const
        p_entry,
        fatfs_entry_info_struct_t *const p_info, uint8_t *const p_buff)
{
    static uint8_t count = 0;
    static uint8_t sub_entry = 0;
    fatfs_entry_type_enum_t entry_type = EMPTY_ENTRY;
    uint8_t i = 0;
    uint8_t j = 0;
    uint32_t temp = 0;

    if (p_entry[0] != EMPTY_ENTRY) /* check if entry is empty */
    {
        if (p_entry[FATFS_MAIN_ENTRY_ATTRIBUTE_OFFSET] !=
                FATFS_SUBENTRY_ATTRIBUTE) /* Main entry */
        {
            /* Parse file name */
            if (0 == p_buff[0])
            {
                for (i = 0; i < FATFS_MAIN_ENTRY_FILE_NAME_BYTES; i++)
                {
                    p_info->file_name[i] =
                        p_entry[FATFS_MAIN_ENTRY_FILE_NAME_OFFSET + i];
                }
                p_info->file_name[i] = '\0';
            }
            else
            {
                for (i = 0; i < sub_entry; i++)
                {
                    for (j = 0; j < FATFS_SUB_ENTRY_DATA_BYTES; j++)
                    {
                        p_info->file_name[i * FATFS_SUB_ENTRY_DATA_BYTES + j] =
                            p_buff[(sub_entry - 1 - i) *
                                   FATFS_SUB_ENTRY_DATA_BYTES +
                                   j];
                    }
                }
                p_buff[0] = '\0';
                count = 0;
                sub_entry = 0;
            }

            /* Parse file attribute */
            p_info->file_attribute = p_entry[FATFS_MAIN_ENTRY_ATTRIBUTE_OFFSET];

            /* Parse file extension */
            for (i = 0; i < FATFS_MAIN_ENTRY_FILE_EXTENSION_BYTES; i++)
            {
                p_info->file_extension[i] =
                    p_entry[FATFS_MAIN_ENTRY_FILE_EXTENSION_OFFSET + i];
            }
            p_info->file_extension[i] = '\0';

            /* Parse modified time */
            temp = (uint32_t)
                   p_entry[FATFS_MAIN_ENTRY_MODIFIED_TIME_OFFSET +
                           FATFS_MAIN_ENTRY_MODIFIED_TIME_BYTES - 1];
            for (i = FATFS_MAIN_ENTRY_MODIFIED_TIME_BYTES - 1; i > 0; i--)
            {
                temp = (uint32_t)make_value_little_endian(
                           p_entry[FATFS_MAIN_ENTRY_MODIFIED_TIME_OFFSET +
                                   i - 1], temp);
            }
            p_info->modified_time.second =
                (uint8_t)(((temp &
                            FATFS_MAIN_ENTRY_MODIFIED_TIME_SECOND_MASK) >>
                           (FATFS_MAIN_ENTRY_MODIFIED_TIME_SECOND_SHIFT)) *
                          2);
            p_info->modified_time.minute =
                (uint8_t)((temp &
                           FATFS_MAIN_ENTRY_MODIFIED_TIME_MINUTE_MASK) >>
                          (FATFS_MAIN_ENTRY_MODIFIED_TIME_MINUTE_SHIFT));
            p_info->modified_time.hour =
                (uint8_t)((temp &
                           FATFS_MAIN_ENTRY_MODIFIED_TIME_HOUR_MASK) >>
                          (FATFS_MAIN_ENTRY_MODIFIED_TIME_HOUR_SHIFT));

            /* Parse modified date */
            temp = (uint32_t)
                   p_entry[FATFS_MAIN_ENTRY_MODIFIED_DATE_OFFSET +
                           FATFS_MAIN_ENTRY_MODIFIED_DATE_BYTES - 1];
            for (i = FATFS_MAIN_ENTRY_MODIFIED_DATE_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           p_entry[FATFS_MAIN_ENTRY_MODIFIED_DATE_OFFSET +
                                   i - 1], temp);
            }
            p_info->modified_date.day =
                (uint8_t)((temp &
                           FATFS_MAIN_ENTRY_MODIFIED_DATE_DAY_MASK) >>
                          (FATFS_MAIN_ENTRY_MODIFIED_DATE_DAY_SHIFT));
            p_info->modified_date.month =
                (uint8_t)((temp &
                           FATFS_MAIN_ENTRY_MODIFIED_DATE_MONTH_MASK) >>
                          (FATFS_MAIN_ENTRY_MODIFIED_DATE_MONTH_SHIFT));
            p_info->modified_date.year =
                (uint16_t)(((temp &
                             FATFS_MAIN_ENTRY_MODIFIED_DATE_YEAR_MASK) >>
                            (FATFS_MAIN_ENTRY_MODIFIED_DATE_YEAR_SHIFT)) +
                           1980);

            /* Parse file size */
            temp = (uint32_t)
                   p_entry[FATFS_MAIN_ENTRY_FILE_SIZE_OFFSET +
                           FATFS_MAIN_ENTRY_FILE_SIZE_BYTES - 1];
            for (i = FATFS_MAIN_ENTRY_FILE_SIZE_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           p_entry[FATFS_MAIN_ENTRY_FILE_SIZE_OFFSET +
                                   i - 1], temp);
            }
            p_info->file_size = temp;
            if (0 == (temp % (s_boot_info.byte_per_sector *
                              s_boot_info.sector_per_cluster)))
            {
                temp = (uint32_t)(temp / (s_boot_info.byte_per_sector *
                                          s_boot_info.sector_per_cluster));
            }
            else
            {
                temp = (uint32_t)((temp / (s_boot_info.byte_per_sector *
                                           s_boot_info.sector_per_cluster)) +
                                  1);
            }

            p_info->file_round_up_size = temp * s_boot_info.byte_per_sector *
                                         s_boot_info.sector_per_cluster;

            /* Parse first cluster */
            temp = (uint32_t)
                   p_entry[FATFS_MAIN_ENTRY_FIRST_CLUSTER_OFFSET +
                           FATFS_MAIN_ENTRY_FIRST_CLUSTER_BYTES - 1];
            for (i = FATFS_MAIN_ENTRY_FIRST_CLUSTER_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           p_entry[FATFS_MAIN_ENTRY_FIRST_CLUSTER_OFFSET +
                                   i - 1], temp);
            }
            p_info->first_cluster = temp;

            p_info->p_next = NULL;
            entry_type = MAIN_ENTRY;
        }
        else /* Sub entry */
        {
            for (i = 0; i < FATFS_SUB_ENTRY_FIRST_FIVE_CHARACTER_BYTES;
                    i = i + 2)
            {
                temp = make_value_little_endian(
                           p_entry[FATFS_SUB_ENTRY_FIRST_FIVE_CHARACTER_OFFSET
                                   + i],
                           p_entry[FATFS_SUB_ENTRY_FIRST_FIVE_CHARACTER_OFFSET
                                   + i + 1]);
                p_buff[count++] = temp;
            }
            for (i = 0; i < FATFS_SUB_ENTRY_NEXT_SIX_CHARACTER_BYTES; i = i + 2)
            {
                temp = make_value_little_endian(
                           p_entry[FATFS_SUB_ENTRY_NEXT_SIX_CHARACTER_OFFSET
                                   + i],
                           p_entry[FATFS_SUB_ENTRY_NEXT_SIX_CHARACTER_OFFSET
                                   + i + 1]);
                p_buff[count++] = temp;
            }
            for (i = 0; i < FATFS_SUB_ENTRY_NEXT_TWO_CHARACTER_BYTES; i = i + 2)
            {
                temp = make_value_little_endian(
                           p_entry[FATFS_SUB_ENTRY_NEXT_TWO_CHARACTER_OFFSET
                                   + i],
                           p_entry[FATFS_SUB_ENTRY_NEXT_TWO_CHARACTER_OFFSET
                                   + i + 1]);
                p_buff[count++] = temp;
            }
            sub_entry++;
        }
    }
    else
    {
        entry_type = EMPTY_ENTRY;
    }

    return entry_type;
}

/* Function is used to get index of next cluster */
static fatfs_error_enum_t fatfs_get_next_cluster(uint32_t *const
        next_cluster)
{
    fatfs_error_enum_t error = SUCCESS;
    uint8_t *p_temp = NULL;
    uint32_t fat_element_index = 0;
    uint32_t temp = 0;
    uint32_t bytes = 0;

    p_temp = (uint8_t *)malloc(s_boot_info.byte_per_sector * 2);
    fat_element_index = (uint32_t)((*next_cluster * s_boot_info.fat_type / 8));
    temp = (uint32_t)(((fat_element_index) / s_boot_info.byte_per_sector));
    bytes = kmc_read_multi_sector(temp + s_boot_info.sector_before_fat, 2,
                                  p_temp);
    if (bytes == s_boot_info.byte_per_sector * 2)
    {
        bytes = fat_element_index - (s_boot_info.byte_per_sector * temp);
        if (s_boot_info.fat_type == 12)
        {
            if (*next_cluster % 2 == 0)
            {
                *next_cluster = make_even_element_fat(p_temp[bytes],
                                                      p_temp[bytes + 1]);
            }
            else
            {
                *next_cluster = make_odd_element_fat(p_temp[bytes],
                                                     p_temp[bytes + 1]);
            }
        }
        else
        {
            *next_cluster = make_value_little_endian(p_temp[bytes],
                            p_temp[bytes + 1]);
        }
    }
    else
    {
        error = FATFS_READ_SECTOR_FAILED;
    }
    free(p_temp);

    return error;
}

/* Function is used to insert entry to list */
static void fatfs_insert(fatfs_entry_info_struct_t *const new_entry)
{
    if (sp_entry_list_head == NULL)
    {
        sp_entry_list_head = new_entry;
        sp_entry_list_tail = new_entry;
    }
    else
    {
        sp_entry_list_tail->p_next = new_entry;
        sp_entry_list_tail = new_entry;
    }
}

/* Function is used to get error message */
uint8_t *getErrorMessage(const fatfs_error_enum_t err)
{
    uint8_t *errorMessage[] =
    {
        "Success",
        "Initialize failed",
        "Read sector failed"
    };

    return errorMessage[err];
}

/* Function is used to initialize FAT */
fatfs_error_enum_t fatfs_init(const uint8_t *const file_path,
                              fatfs_boot_sector_struct_t **const p_boot)
{
    fatfs_error_enum_t error = SUCCESS;
    uint8_t boot_sector[FATFS_BOOT_SECTOR_SIZE] = {};
    uint8_t fat_type[FATFS_FAT_TYPE_SIZE + 1];
    uint8_t i = 0;
    uint32_t temp = 0;
    uint8_t number_fat = 0;
    uint16_t sector_per_fat = 0;
    uint16_t root_entry = 0;

    if (true == kmc_init(file_path))
    {
        if (FATFS_BOOT_SECTOR_SIZE == kmc_read_sector(FATFS_BOOT_SECTOR_INDEX,
                boot_sector))
        {
            /* Read sector before fat */
            temp = (uint32_t)
                   boot_sector[FATFS_NUMBER_SECTOR_BEFORE_FAT_OFFSET +
                               FATFS_NUMBER_SECTOR_BEFORE_FAT_BYTES - 1];
            for (i = FATFS_NUMBER_SECTOR_BEFORE_FAT_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           boot_sector[FATFS_NUMBER_SECTOR_BEFORE_FAT_OFFSET +
                                       i - 1], temp);
            }
            s_boot_info.sector_before_fat = (uint16_t)temp;

            /* Read byte per sector */
            temp = (uint32_t)
                   boot_sector[FATFS_BYTE_PER_SECTOR_OFFSET +
                               FATFS_BYTE_PER_SECTOR_BYTES - 1];
            for (i = FATFS_BYTE_PER_SECTOR_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           boot_sector[FATFS_BYTE_PER_SECTOR_OFFSET +
                                       i - 1], temp);
            }
            s_boot_info.byte_per_sector = (uint16_t)temp;

            /* Read number FAT */
            temp = (uint32_t)
                   boot_sector[FATFS_NUMBER_FAT_OFFSET +
                               FATFS_NUMBER_FAT_BYTES - 1];
            for (i = FATFS_NUMBER_FAT_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           boot_sector[FATFS_NUMBER_FAT_OFFSET + i - 1], temp);
            }
            number_fat = (uint8_t)temp;

            /* Read sector per FAT */
            temp = (uint32_t)
                   boot_sector[FATFS_SECTOR_PER_FAT_OFFSET +
                               FATFS_SECTOR_PER_FAT_BYTES - 1];
            for (i = FATFS_SECTOR_PER_FAT_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           boot_sector[FATFS_SECTOR_PER_FAT_OFFSET +
                                       i - 1], temp);
            }
            sector_per_fat = (uint16_t)temp;

            /* Read root directory index */
            temp = s_boot_info.sector_before_fat + sector_per_fat *
                   (uint16_t)number_fat;
            s_boot_info.root_directory_index = temp;

            /* Read root entry */
            temp = (uint32_t)
                   boot_sector[FATFS_ROOT_ENTRY_OFFSET +
                               FATFS_ROOT_ENTRY_BYTES - 1];
            for (i = FATFS_ROOT_ENTRY_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           boot_sector[FATFS_ROOT_ENTRY_OFFSET + i - 1], temp);
            }
            root_entry = (uint16_t)temp;

            /* Read sector per cluster */
            temp = (uint32_t)
                   boot_sector[FATFS_SECTOR_PER_CLUSTER_OFFSET +
                               FATFS_SECTOR_PER_CLUSTER_BYTES - 1];
            for (i = FATFS_SECTOR_PER_CLUSTER_BYTES - 1; i > 0; i--)
            {
                temp = make_value_little_endian(
                           boot_sector[FATFS_SECTOR_PER_CLUSTER_OFFSET +
                                       i - 1], temp);
            }
            s_boot_info.sector_per_cluster = (uint8_t)temp;

            /* Read data index */
            temp = (uint32_t)(root_entry * FATFS_ENTRY_SIZE /
                              s_boot_info.byte_per_sector);
            s_boot_info.data_index =
                (uint32_t)(s_boot_info.root_directory_index +
                           temp);

            /* Read fat type */
            for (i = 0; i < FATFS_FAT_TYPE_SIZE; i++)
            {
                fat_type[i] = boot_sector[FATFS_FAT_TYPE_OFFSET + i];
            }
            fat_type[i] = '\0';
            if (fat_type[4] == '2')
            {
                s_boot_info.fat_type = 12;
                s_end_cluster = 0xFFF;
            }
            else if (fat_type[4] == '6')
            {
                s_boot_info.fat_type = 16;
                s_end_cluster = 0xFFFF;
            }
            else
            {
                s_boot_info.fat_type = 32;
            }
        }
        else
        {
            error = FATFS_READ_SECTOR_FAILED;
        }
    }
    else
    {
        error = FATFS_INITIALIZE_FAILED;
    }
    kmc_update_sector_size(s_boot_info.byte_per_sector);
    *p_boot = &s_boot_info;

    return error;
}

/* Function is used to read directory */
fatfs_error_enum_t fatfs_read_directory(const uint32_t first_cluster,
                   fatfs_entry_info_struct_t **const p_list)
{
    fatfs_error_enum_t error = SUCCESS;
    uint32_t bytes = 0;
    uint32_t i = 0;
    uint8_t j = 0;
    uint32_t next_cluster = first_cluster;
    fatfs_entry_info_struct_t *p_entry_info = NULL;
    uint8_t *p_buff = NULL;
    fatfs_entry_type_enum_t entry_type = 0;
    uint8_t *p_name = NULL;

    free(sp_entry_list_head);
    sp_entry_list_head = NULL;
    sp_entry_list_tail = NULL;
    if (0 == first_cluster) /* Read root directory */
    {
        p_buff = malloc(
               (s_boot_info.data_index - s_boot_info.root_directory_index) *
                     s_boot_info.byte_per_sector);
        bytes = kmc_read_multi_sector(s_boot_info.root_directory_index,
        (s_boot_info.data_index - s_boot_info.root_directory_index), p_buff);
        if (bytes ==
                (s_boot_info.data_index - s_boot_info.root_directory_index) *
                s_boot_info.byte_per_sector)
        {
            p_entry_info = (fatfs_entry_info_struct_t *)malloc(sizeof(
                               fatfs_entry_info_struct_t));
            p_name = (uint8_t *)malloc(256);
            p_name[0] = '\0';
            for (i = 0; i < bytes / FATFS_ENTRY_SIZE; i++)
            {
                entry_type = fatfs_decode_entry(p_buff + i * FATFS_ENTRY_SIZE,
                                                p_entry_info, p_name);
                if (entry_type == MAIN_ENTRY)
                {
                    if ((p_entry_info->file_attribute == 0x00) ||
                            (p_entry_info->file_attribute == 0x10) ||
                            (p_entry_info->file_attribute == 0x20))
                    {
                        fatfs_insert(p_entry_info);
                        p_entry_info =
                            (fatfs_entry_info_struct_t *)malloc(sizeof(
                                    fatfs_entry_info_struct_t));
                    }
                }
            }
            free(p_entry_info);
            free(p_name);
            free(p_buff);
        }
        else
        {
            error = FATFS_READ_SECTOR_FAILED;
        }
    }
    else
    {
        p_buff = malloc(s_boot_info.byte_per_sector *
                        s_boot_info.sector_per_cluster);
        while (next_cluster != s_end_cluster)
        {
            bytes = kmc_read_multi_sector((next_cluster - 2) *
                                          s_boot_info.sector_per_cluster +
                                          s_boot_info.data_index,
                                          s_boot_info.sector_per_cluster,
                                          p_buff);
            if (bytes == s_boot_info.sector_per_cluster *
                    s_boot_info.byte_per_sector)
            {
                p_entry_info = (fatfs_entry_info_struct_t *)malloc(sizeof(
                                   fatfs_entry_info_struct_t));
                p_name = (uint8_t *)malloc(256);
                p_name[0] = '\0';
                for (i = 0; i < bytes / FATFS_ENTRY_SIZE; i++)
                {
                    entry_type = fatfs_decode_entry(p_buff +
                                                    i * FATFS_ENTRY_SIZE,
                                                    p_entry_info, p_name);
                    if (entry_type == MAIN_ENTRY)
                    {

                        if ((p_entry_info->file_attribute == 0x00) ||
                                (p_entry_info->file_attribute == 0x10) ||
                                (p_entry_info->file_attribute == 0x20))
                        {
                            if (strcmp(p_entry_info->file_name, ".       ")
                                    != 0)
                            {
                                fatfs_insert(p_entry_info);
                                p_entry_info =
                                    (fatfs_entry_info_struct_t *)malloc(sizeof(
                                            fatfs_entry_info_struct_t));
                            }
                            else
                            {
                                /* Do nothing */
                            }
                        }
                        else
                        {
                            /* Do nothing */
                        }
                    }
                }
                free(p_entry_info);
                free(p_name);
                error = fatfs_get_next_cluster(&next_cluster);
                if (error != SUCCESS)
                {
                    next_cluster = s_end_cluster;
                }
                else
                {
                    /* Do nothing */
                }
            }
            else
            {
                error = FATFS_READ_SECTOR_FAILED;
            }
        }
        free(p_buff);
    }
    *p_list = sp_entry_list_head;

    return error;
}

/* Function is used to read file */
fatfs_error_enum_t fatfs_read_file(uint32_t first_cluster, uint8_t *p_buff)
{
    fatfs_error_enum_t error = SUCCESS;
    uint32_t next_cluster = first_cluster;
    uint32_t bytes = 0;
    uint16_t i = 0;
    uint32_t fat_element_index = 0;
    uint32_t temp = 0;
    uint16_t j = 0;

    while (next_cluster != s_end_cluster)
    {
        temp = s_boot_info.byte_per_sector * s_boot_info.sector_per_cluster;
        bytes = kmc_read_multi_sector((next_cluster - 2) *
                                      s_boot_info.sector_per_cluster +
                                      s_boot_info.data_index,
                                      s_boot_info.sector_per_cluster,
                                      &p_buff[temp * i]);
        if (bytes == s_boot_info.sector_per_cluster *
                s_boot_info.byte_per_sector)
        {
            i++;
            error = fatfs_get_next_cluster(&next_cluster);
            if (error != SUCCESS)
            {
                next_cluster = s_end_cluster;
            }
            else
            {
                /* Do nothing */
            }
        }
        else
        {
            error = FATFS_READ_SECTOR_FAILED;
        }
    }

    return error;
}

/* Function is used to de-initialize FAT */
void fatfs_deinit(void)
{
    kmc_deinit();
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
