#include <fcntl.h>
#include <unistd.h>
#include <libzbd/zbd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

const char *dev_path = "/dev/sde";

int main() {
    struct zbd_info info;
    struct zbd_zone *zones;
    if(!zbd_device_is_zoned(dev_path)){
        printf("the device is not zoned device");
        return -1;
    }
// zbd_open takes path , read and all the acess and struct zbd_info which will give all the info abt the zbd
    int fd = zbd_open(dev_path, O_RDONLY , &info);
    if (fd < 0) {
        perror("open failed");
        return 1;
    }
// zbd_get_info and zbd_info do the same this in the fuction veen we just copy the zbd_info into the zbd_get_info , so  mainly it used id the device is opened by other means

/*if (zbd_get_info(fd, &info) != 0) {
        perror("zbd_get_info failed");
        close(fd);
        return 1;
    }*/
    printf("Device model: %s\n", zbd_device_model_str(info.model, true));
    printf("Vendor ID: %s\n", info.vendor_id);
    printf("Number of sectors: %llu\n", info.nr_sectors);
    printf("Number of logical blocks: %llu\n", info.nr_lblocks);
    printf("Number of physical blocks: %llu\n", info.nr_pblocks);
    printf("Zone size (bytes): %llu\n", info.zone_size);
    printf("Zone size (sectors): %u\n", info.zone_sectors);
    printf("Logical block size (bytes): %u\n", info.lblock_size);
    printf("Physical block size (bytes): %u\n", info.pblock_size);
    printf("Number of zones: %u\n", info.nr_zones);
    printf("Max number of open zones: %u\n", info.max_nr_open_zones);
    printf("Max number of active zones: %u\n", info.max_nr_active_zones);
    printf("Model: %u\n", info.model);
    unsigned int nr =info.nr_zones;
    zones=calloc(nr , sizeof(struct zbd_zone)) ;
    if(!zones){
        printf("calloc failed");
        close(fd);
        return 1;
    }
   int  ret = zbd_list_zones(fd, 0, info.nr_sectors * 512ULL, ZBD_RO_ALL, &zones, &nr);
    if (ret != 0) {
        perror("zbd_list_zones failed");
        free(zones);
        close(fd);
        return 1;
    }

    for(unsigned int i=0;i<nr ;i++){
      printf("Zone %u: start=%llu, len=%llu, type=%u, cond=%u , capa=%llu , writer_pointer=%llu, flags=%u \n",i,
               zones[i].start,
               zones[i].len,
               zones[i].type,
               zones[i].cond,
               zones[i].capacity,
            zones[i].wp,
        zones[i].flags);
    }
    free(zones);
    zbd_close(fd);
    return 0;
}
