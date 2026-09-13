#ifndef ZB_PORT_FS_H
#define ZB_PORT_FS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct zb_port_file zb_port_file_t;
typedef struct zb_port_dir zb_port_dir_t;

struct zb_port_dirent {
    char d_name[64];
};

struct zb_port_stat {
    uint32_t st_size;
    uint8_t st_mode;
};

zb_port_file_t *zb_port_fopen(const char *path, const char *mode);
size_t zb_port_fread(void *ptr, size_t size, size_t nmemb, zb_port_file_t *f);
size_t zb_port_fwrite(const void *ptr, size_t size, size_t nmemb, zb_port_file_t *f);
int zb_port_fclose(zb_port_file_t *f);

zb_port_dir_t *zb_port_opendir(const char *path);
struct zb_port_dirent *zb_port_readdir(zb_port_dir_t *d);
int zb_port_closedir(zb_port_dir_t *d);

int zb_port_remove(const char *path);
int zb_port_rename(const char *from, const char *to);
int zb_port_mkdir(const char *path);
int zb_port_stat(const char *path, struct zb_port_stat *st);
int zb_port_exists(const char *path);

int zb_port_file_size(const char *path);
int zb_port_file_read(const char *path, uint8_t *data, uint16_t size, uint32_t offset);
int zb_port_file_write(const char *path, const uint8_t *data, uint16_t size, uint32_t offset);

int iotdev_is_file_exists(const char *path);
int iotdev_create_dir(const char *path);

uint32_t zb_port_crc32(uint32_t crc, const void *data, size_t size);

#ifdef ZB_USE_VFS
#define FILE zb_port_file_t
#define fopen zb_port_fopen
#define fread zb_port_fread
#define fwrite zb_port_fwrite
#define fclose zb_port_fclose
#define DIR zb_port_dir_t
#define dirent zb_port_dirent
#define opendir zb_port_opendir
#define readdir zb_port_readdir
#define closedir zb_port_closedir
#define remove zb_port_remove
#define rename zb_port_rename
#define mkdir(path, mode) zb_port_mkdir(path)
#define stat(path, st) zb_port_stat((path), (struct zb_port_stat *)(st))
struct stat {
    uint32_t st_size;
    uint8_t st_mode;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZB_PORT_FS_H */
