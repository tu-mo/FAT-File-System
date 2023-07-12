#ifndef _FAT_H_
#define _FAT_H_

/*******************************************************************************
 * Includes
 ******************************************************************************/
typedef struct
{
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} fatfs_modified_time_struct_t;

typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
} fatfs_modified_date_struct_t;

typedef struct _entry_info
{
    uint8_t file_name[256];
    uint8_t file_extension[4];
    uint8_t file_attribute;
    fatfs_modified_time_struct_t modified_time;
    fatfs_modified_date_struct_t modified_date;
    uint32_t file_size;
    uint32_t file_round_up_size;
    uint32_t first_cluster;
    struct _entry_info *p_next;
} fatfs_entry_info_struct_t;

typedef struct
{
    uint16_t sector_before_fat;
    uint16_t byte_per_sector;
    uint32_t root_directory_index;
    uint32_t data_index;
    uint8_t sector_per_cluster;
    uint8_t fat_type;
} fatfs_boot_sector_struct_t;

typedef enum
{
    SUCCESS,
    FATFS_INITIALIZE_FAILED,
    FATFS_READ_SECTOR_FAILED
} fatfs_error_enum_t;

/*******************************************************************************
 * API
 ******************************************************************************/
/**
 * @brief Initialize FAT
 *
 * @param [in] file_path is path to file
 * @param [out] p_boot is boot sector
 * @return fatfs_error_enum_t is error code
 */
fatfs_error_enum_t fatfs_init(const uint8_t *const file_path,
                              fatfs_boot_sector_struct_t **const p_boot);

/**
 * @brief Read directory
 *
 * @param [in] first_cluster is first cluster of directory
 * @param [out] p_list is entry list
 * @return fatfs_error_enum_t is error code
 */
fatfs_error_enum_t fatfs_read_directory(const uint32_t first_cluster,
                                        fatfs_entry_info_struct_t **const p_list);

/**
 * @brief Read file
 *
 * @param [in] first_cluster is first cluster of file
 * @param [out] p_buff is content of file
 * @return fatfs_error_enum_t is error code
 */
fatfs_error_enum_t fatfs_read_file(const uint32_t first_cluster,
                                   uint8_t *const p_buff);

/**
 * @brief Get error Message
 *
 * @param [in] err is error type
 * @return uint8_t* is content of error
 */
uint8_t *getErrorMessage(const fatfs_error_enum_t err);

/**
 * @brief De-initialize FAT
 *
 */
void fatfs_deinit(void);

#endif /* _FAT_H_ */

/*******************************************************************************
 * EOF
 ******************************************************************************/
