#include <libzbd/zbd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/sde"
#define BUFFER_SIZE 4096
#define SECTOR_SIZE 512
#define ZONE_INDEX 5
int sequential_zone_read_all(int fd, struct zbd_zone *zone) {
    if (zbd_zone_cnv(zone)) {
        printf("Zone is conventional. Use random read instead.\n");
        return -1;
    }

    uint64_t offset = zone->start;
    uint64_t length = zbd_zone_wp(zone)- zone->start;

    if (length == 0) {
        printf("Zone is empty. Nothing to read.\n");
        return 0;
    }

    char *buffer = malloc(length + 1);
    if (!buffer) {
        perror("malloc failed");
        return -1;
    }

    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek failed");
        free(buffer);
        return -1;
    }

    ssize_t ret = read(fd, buffer, length);
    if (ret < 0) {
        perror("read failed");
        free(buffer);
        return -1;
    }

    buffer[ret] = '\0';  
    printf("Read %zd bytes from zone:\n%s\n", ret, buffer);

    free(buffer);
    return ret;
}


int sequential_zone_write(int fd, struct zbd_zone *zone, const char *data, unsigned int *nr)
{
    ssize_t ret;
    uint64_t offset;

    if (zbd_zone_cnv(zone)) {
        printf("Zone is conventional. Use random write instead.\n");
        return -1;
    }

    if (zbd_zone_full(zone)) {
        char response;
        printf("Zone is full. Do you want to reset it? (y/n): ");
        scanf(" %c", &response);
        if (response == 'y') {
            if (zbd_reset_zones(fd, zone->start, 1) < 0) {
                perror("Failed to reset zone");
                return -1;
            }
            printf("Zone reset successfully.\n");
            zone->wp = zone->start;
        } else {
            printf("Zone not reset. Aborting write.\n");
            return -1;
        }
    }

    if (!zbd_zone_is_open(zone)) {
        if (zbd_open_zones(fd, zone->start, 1) < 0) {
            perror("Failed to open zone");
            return -1;
        }
        printf("Zone opened successfully.\n");
    }

    // Refresh zone info before writing
    if (zbd_report_zones(fd, zone->start, 1, ZBD_RO_ALL, zone, nr) < 0) {
        perror("Failed to refresh zone");
        return -1;
    }

    offset = zbd_zone_wp(zone);
    printf("Writing at offset: %lu\n", (unsigned long)offset);

    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek failed");
        return -1;
    }

    ret = write(fd, data, strlen(data));
    if (ret < 0) {
        perror("Write failed");
        return -1;
    }

    printf("Updated write pointer: %lu\n", (unsigned long)zbd_zone_wp(zone));
    printf("Wrote %zd bytes to zone at offset %lu.\n", ret, (unsigned long)offset);

    return ret;
}


int main() {
    struct zbd_info info;
    struct zbd_zone *zones;
    int fd;

    fd = zbd_open(DEVICE, O_RDWR, &info);
    if (fd < 0) {
        perror("zbd_open");
        return 1;
    }

    unsigned int nr = info.nr_zones;
    zones = calloc(nr, sizeof(struct zbd_zone));
    if (!zones) {
        perror("calloc failed");
        zbd_close(fd);
        return 1;
    }

    if (zbd_report_zones(fd, 0, info.nr_sectors * SECTOR_SIZE, ZBD_RO_ALL, zones, &nr) != 0) {
        perror("zbd_report_zones failed");
        free(zones);
        zbd_close(fd);
        return 1;
    }

    char write_data[] = "checking second write one more!";
    
int i = sequential_zone_write(fd, &zones[ZONE_INDEX], write_data, &nr);
if (i < 0) {
     fprintf(stderr, "zbd write failed\n");
}

sequential_zone_write(fd, &zones[ZONE_INDEX], " one more!", &nr);

 int i1=sequential_zone_read_all(fd , &zones[ZONE_INDEX]);
 if (i1< 0) {
        fprintf(stderr, "zbd write failed\n");
    }

    free(zones);
    zbd_close(fd);
    return 0;
}
