/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fat.h"

/*******************************************************************************
 * Definition
 ******************************************************************************/
#define FILE_PATH "floppy.img"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/**
 * @brief Make option for user choose
 *
 * @param [inout] p_directory_list is directory entry list
 */
static void utility_make_option(fatfs_entry_info_struct_t
                                *p_directory_list);

/**
 * @brief Make content of file
 *
 * @param [in] buff is data of file
 * @param [in] file_size is size of file
 */
static void make_content_file(const uint8_t *const buff,
                              const uint32_t file_size);

/*******************************************************************************
 * Code
 ******************************************************************************/
/* Function is used to make content of directory */
static void utility_make_option(fatfs_entry_info_struct_t
                                *p_directory_list)
{
    uint8_t i = 0;

    printf("%-5s%-30s%-20s%-10s%s", "No", "Name", "Type", "Size",
           "Date modified");
    while (p_directory_list != NULL)
    {
        printf("\n%-5d%-30s", i++, p_directory_list->file_name);
        if (strcmp(p_directory_list->file_name, "..      ") != 0)
        {
            if (0 == strcmp(p_directory_list->file_extension, "   "))
            {
                printf("%-20s", "File Folder");
                printf("%-10s", "");
            }
            else
            {
                printf("%s %-16s", p_directory_list->file_extension, "File");
                printf("%-10d", p_directory_list->file_size);
            }
            printf("%d/%d/%d", p_directory_list->modified_date.day,
                   p_directory_list->modified_date.month,
                   p_directory_list->modified_date.year);
            printf(" %d:%d:%d", p_directory_list->modified_time.hour,
                   p_directory_list->modified_time.minute,
                   p_directory_list->modified_time.second);
        }
        else
        {
            /* Do nothing */
        }
        p_directory_list = p_directory_list->p_next;
    }
}

/* Function is used to make content of file */
static void make_content_file(const uint8_t *const buff,
                              const uint32_t file_size)
{
    uint32_t i = 0;

    for (i = 0; i < file_size; i++)
    {
        printf("%c", buff[i]);
    }
    printf("\n");
}

/* Main function */
int main()
{
    fatfs_entry_info_struct_t *p_directory_list = NULL;
    fatfs_boot_sector_struct_t *p_boot;
    uint8_t *buff = NULL;
    uint32_t select = 0;
    uint32_t i = 0;
    fatfs_error_enum_t error = SUCCESS;
    fatfs_entry_info_struct_t *temp = NULL;

    if (SUCCESS == fatfs_init(FILE_PATH, &p_boot))
    {
        fatfs_read_directory(0, &p_directory_list);
        utility_make_option(p_directory_list);
        while (true)
        {
            printf("\nSelect: ");
            scanf("%d", &select);
            system("cls");
            temp = p_directory_list;
            for (i = 0; i < select; i++)
            {
                p_directory_list = p_directory_list->p_next;
            }
            if (0 == strcmp(p_directory_list->file_extension, "   "))
            {
                fatfs_read_directory(p_directory_list->first_cluster,
                                     &p_directory_list);
                utility_make_option(p_directory_list);
            }
            else
            {
                buff = (uint8_t *)malloc(p_directory_list->file_round_up_size);
                fatfs_read_file(p_directory_list->first_cluster, buff);
                make_content_file(buff, p_directory_list->file_size);
                fflush(stdin);
                getchar();
                system("cls");
                p_directory_list = temp;
                utility_make_option(p_directory_list);
                free(buff);
            }
        }
    }
    fatfs_deinit();

    return 0;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
