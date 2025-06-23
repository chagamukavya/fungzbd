#include <libzbd/zbd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/sde"
#define BUFFER_SIZE 4096
#define SECTOR_SIZE 512
#define ZONE_INDEX 7

int sequential_zone_read_all(int fd, struct zbd_zone *zone) {
    if (zbd_zone_cnv(zone)) {
        printf("Zone is conventional. Use random read instead.\n");
        return -1;
    }

    uint64_t offset = zone->start;
    uint64_t wp = zbd_zone_wp(zone);
    uint64_t length = wp - offset;

    if (length == 0) {
        printf("Zone is empty. Nothing to read.\n");
        return 0;
    }

    char buffer[BUFFER_SIZE + 1];
    ssize_t ret;
    uint64_t total_read = 0;

    if (lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek failed");
        return -1;
    }

    printf("Reading %lu bytes from zone...\n", length);
    while (total_read < length) {
        size_t to_read = (length - total_read > BUFFER_SIZE) ? BUFFER_SIZE : (length - total_read);
        ret = read(fd, buffer, to_read);
        if (ret < 0) {
            perror("read failed");
            return -1;
        }
        buffer[ret] = '\0';
        printf("%s", buffer);
        total_read += ret;
    }

    printf("\nTotal read: %lu bytes\n", total_read);
    return total_read;
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

    ret = pwrite(fd, data, strlen(data), offset );
    if (ret < 0) {
        perror("Failed to write to zone");
        free(zone);
        zbd_close(fd);
        return EXIT_FAILURE;
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

   int  ret = zbd_list_zones(fd, 0, info.nr_sectors * 512ULL, ZBD_RO_ALL, &zones, &nr);
        if (ret != 0) {
        perror("zbd_list_zones failed");
        free(zones);
        close(fd);
        return 1;
    }

printf("Zone %d: start=%lu, len=%lu, wp=%lu, type=%s\n", 
       ZONE_INDEX,
       zones[ZONE_INDEX].start,
       zones[ZONE_INDEX].len,
       zbd_zone_wp(&zones[ZONE_INDEX]),
       zbd_zone_cnv(&zones[ZONE_INDEX]) ? "conventional" : "sequential");

    char write_data[] = "checking second write one more this is on 7!";
    
int i = sequential_zone_write(fd, &zones[ZONE_INDEX], write_data, &nr);
if (i < 0) {
     fprintf(stderr, "zbd write failed\n");
}
fsync(fd);
int i1=sequential_zone_write(fd, &zones[ZONE_INDEX], " one more on 7th!", &nr);
if(i<0){
 printf(" this is failed seonc one");
}
fsync(fd);
int i2=sequential_zone_write(fd, &zones[ZONE_INDEX], " this is shit on 7th!", &nr);
if(i2<0){
    printf("this shit also failed");
}
fsync(fd);
int i3=sequential_zone_read_all(fd, &zones[ZONE_INDEX]);
if(i3<0){
    printf("read also failed");
}
    free(zones);
    zbd_close(fd);
    return 0;
}
