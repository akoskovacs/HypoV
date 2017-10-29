#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>

static void show_help(const char *prog_name)
{
    printf("Usage: %s -l loader -o output_fs\n", prog_name);
    printf("\t-l loader\t\tStage 0 VBR loader image to install\n");
    printf("\t-o filesystem\t\tIntallation target FAT32 filesystem (device or regular file)\n\n");
}

static bool is_fat32(uint8_t *block0)
{
    return false;
}

static off_t get_loader_offset(uint8_t *block0)
{
    return 0L;
}

int main(int argc, const char *argv[])
{
    int opt;
    is_fat32(NULL);
    get_loader_offset(NULL);
    while ((opt = getopt(argc, argv, "hl:i:o:")) != -1) {
        switch (opt) {
        case 'h':
            show_help(argv[0]);
        return 0;

        case 'i': case 'o':
        break;

        default:
            break;
        } 
    }
    return 0;
}
