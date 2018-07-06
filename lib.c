#include <stdbool.h>
#include <blkid/blkid.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "bcache.h"
#include "lib.h"
#include <uuid/uuid.h>
#include <string.h>
#include <malloc.h>


/*
 * utils function
 */

static void trim_prefix(char *dest, char *src, int num)
{
	strcpy(dest, src + num);
}

static void get_tail(char *dest, char *src, int n)
{
	int num, i;
	num = strlen(src);
	for (i = 0; i < n; i++) {
		dest[i] = src[num - n + i];
	}
	dest[i] = '\0';
}

static void trim_tail(char *dest, char *src, int n)
{


	int num, i;
	num = strlen(src);

	for (i = 0; i < num - n; i++) {
		dest[i] = src[i];
	}
	dest[i] = '\0';
}

int get_state(struct dev *dev, char *state)
{
	if (dev->version == BCACHE_SB_VERSION_CDEV
	    || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID) {
		return get_cachedev_state(dev->cset, state);
	} else if (dev->version == BCACHE_SB_VERSION_BDEV
		   || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET) {
		return get_backdev_state(dev->name, state);
	}
}


int get_backdev_state(char *devname, char *state)
{
	FILE *fd;
	char path[100];
	char buf[40];
	trim_prefix(buf, devname, 5);
	sprintf(path, "/sys/block/%s/bcache/state", buf);
	fd = fopen(path, "r");
	if (fd == NULL) {
		strcpy(state, "inactive");
		return 0;
	}
	int i = 0;
	while ((state[i] = getc(fd)) != '\n') {
		i++;
	}
	state[i] = '\0';
	fclose(fd);
	return 0;
}

int get_cachedev_state(char *cset_id, char *state)
{
	int fd;
	char path[100];
	sprintf(path, "/sys/fs/bcache/%s/unregister", cset_id);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		strcpy(state, "inactive");
	} else {
		strcpy(state, "active");
	}
	return 0;
}

int get_bname(struct dev *dev, char *bname)
{
	if (dev->version == BCACHE_SB_VERSION_CDEV
	    || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID) {
		strcpy(bname, "N/A");
	} else if (dev->version == BCACHE_SB_VERSION_BDEV
		   || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET) {
		return get_dev_bname(dev->name, bname);
	}
	return 0;
}

int get_dev_bname(char *devname, char *bname)
{
	int ret;
	char path[100];
	char buf[40];
	char link[100];
	char buf1[100];
	trim_prefix(buf, devname, 5);
	sprintf(path, "/sys/block/%s/bcache/dev", buf);
	ret = readlink(path, link, sizeof(link));
	if (ret < 0) {
		strcpy(bname, "non-exsit");
	} else {
		trim_tail(buf1, link, strlen(link) - ret);
		strcpy(bname, buf1 + 41);
	}
	return 0;
}

int get_point(struct dev *dev, char *point)
{
	if (dev->version == BCACHE_SB_VERSION_CDEV
	    || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID) {
		strcpy(point, "N/A");
	} else if (dev->version == BCACHE_SB_VERSION_BDEV
		   || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET) {
		return get_backdev_attachpoint(dev->name, point);
	}
	return 0;
}

int get_backdev_attachpoint(char *devname, char *point)
{
	int ret;
	char path[100];
	char buf[20];
	char link[100];
	char uuid[40];
	char buf1[100];
	trim_prefix(buf, devname, 5);
	sprintf(path, "/sys/block/%s/bcache/cache", buf);
	ret = readlink(path, link, sizeof(link));
	if (ret < 0) {
		strcpy(point, "alone");
	} else {
		trim_tail(buf1, link, strlen(link) - ret);
		get_tail(uuid, buf1, 36);
		strcpy(point, uuid);
	}
	return 0;
}

int cset_to_devname(struct dev *devs, char *cset, char *devname)
{
	while (devs) {
		if ((devs->version == BCACHE_SB_VERSION_CDEV
		     || devs->version == BCACHE_SB_VERSION_CDEV_WITH_UUID)
		    && strcmp(devs->cset, cset) == 0) {
			strcpy(devname, devs->name);
			break;
		}
		devs = devs->next;
	}
}




int list_bdevs(struct dev **devs)
{
	DIR *dir;
	struct dirent *ptr;
	blkid_probe pr;
	struct cache_sb sb;
	dir = opendir("/sys/block");
	while ((ptr = readdir(dir)) != NULL) {
		if (strcmp(ptr->d_name, ".") == 0
		    || strcmp(ptr->d_name, "..") == 0) {
			continue;
		}
		char dev[20];
		sprintf(dev, "/dev/%s", ptr->d_name);
		int fd = open(dev, O_RDONLY);
		if (fd == -1)
			continue;
		if (!(pr = blkid_new_probe()))
			continue;
		if (blkid_probe_set_device(pr, fd, 0, 0))
			continue;
		if (blkid_probe_enable_partitions(pr, true))
			continue;
		if (!blkid_do_probe(pr)) {
			continue;
		}

		if (pread(fd, &sb, sizeof(sb), SB_START) != sizeof(sb))
			continue;

		if (memcmp(sb.magic, bcache_magic, 16))
			continue;


		struct dev *tmp, *current;
		int ret;
		if (*devs) {
			tmp = (struct dev *) malloc(DEVLEN);
			ret = detail_base(dev, sb, tmp);
			if (ret != 0) {
				printf("error occur");
				return 1;
			} else {
				current->next = tmp;
				current = tmp;
			}
		} else {
			tmp = (struct dev *) malloc(DEVLEN);
			ret = detail_base(dev, sb, tmp);
			if (ret != 0) {
				printf("error occur");
				return 1;
			}
			current = tmp;
			*devs = tmp;
		}
	}
	closedir(dir);
	return 0;

}


int detail_bdev(char *devname, struct bdev *bd)
{
	struct cache_sb sb;
	uint64_t expected_csum;
	int fd = open(devname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr,
			"Can't open dev %s,in function:detail_bdev\n",
			devname);
		return 1;
	}

	if (pread(fd, &sb, sizeof(sb), SB_START) != sizeof(sb)) {
		fprintf(stderr, "Couldn't read\n");
		return 1;
	}

	if (!memcmp(sb.magic, bcache_magic, 16)) {
		bd->base.magic = "ok";
	} else {
		bd->base.magic = "bad magic";
		return 1;
	}

	if (sb.offset == SB_SECTOR) {
		bd->base.first_sector = SB_SECTOR;
	} else {
		fprintf(stderr, "Invalid superblock (bad sector)\n");
		return 1;
	}

	expected_csum = csum_set(&sb);
	if (sb.csum == expected_csum) {
		bd->base.csum = sb.csum;
	} else {
		return 1;
	}

	bd->base.version = sb.version;

	strncpy(bd->base.label, (char *) sb.label, SB_LABEL_SIZE);
	bd->base.label[SB_LABEL_SIZE] = '\0';
	if (*bd->base.label)
		printf("a");
	else
		strncpy(bd->base.label, "(empty)", 7);

	uuid_unparse(sb.uuid, bd->base.uuid);

	bd->base.sectors_per_block = sb.block_size;
	bd->base.sectors_per_bucket = sb.bucket_size;
	close(fd);
	return 0;
}

int detail_base(char *devname, struct cache_sb sb, struct dev *base)
{
	int ret;
	uint64_t expected_csum;
	strcpy(base->name, devname);
	base->magic = "ok";
	base->first_sector = SB_SECTOR;
	base->csum = sb.csum;
	base->version = sb.version;

	strncpy(base->label, (char *) sb.label, SB_LABEL_SIZE);
	base->label[SB_LABEL_SIZE] = '\0';
	if (*base->label)
		printf("a");
	else
		strncpy(base->label, "(empty)", sizeof(base->label));

	uuid_unparse(sb.uuid, base->uuid);
	uuid_unparse(sb.set_uuid, base->cset);
	base->sectors_per_block = sb.block_size;
	base->sectors_per_bucket = sb.bucket_size;
	if ((ret = get_state(base, base->state)) != 0) {
		printf("err when get state");
		return ret;
	}
	if ((ret = get_bname(base, base->bname)) != 0) {
		printf("err when get bname");
		return ret;
	}
	if ((ret = get_point(base, base->attachuuid)) != 0) {
		printf("err when get attach");
		return ret;
	}
	return 0;
}

int detail_dev(char *devname, struct bdev *bd, struct cdev *cd, int *type)
{
	struct cache_sb sb;
	uint64_t expected_csum;
	int fd = open(devname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: Can't open dev  %s\n", devname);
		return 1;
	}

	if (pread(fd, &sb, sizeof(sb), SB_START) != sizeof(sb)) {
		fprintf(stderr, "Couldn't read\n");
		return 1;
	}

	if (memcmp(sb.magic, bcache_magic, 16)) {
		fprintf(stderr,
			"Bad magic,make sure this is an bcache device\n");
		return 1;
	}

	if (!sb.offset == SB_SECTOR) {
		fprintf(stderr, "Invalid superblock (bad sector)\n");
		return 1;
	}

	expected_csum = csum_set(&sb);
	if (!sb.csum == expected_csum) {
		fprintf(stderr, "Csum is not match with expected one");
		return 1;
	}

	*type = sb.version;
	if (sb.version == BCACHE_SB_VERSION_BDEV) {
		detail_base(devname, sb, &bd->base);
		bd->first_sector = BDEV_DATA_START_DEFAULT;
		bd->cache_mode = BDEV_CACHE_MODE(&sb);
		bd->cache_state = BDEV_STATE(&sb);
	} else if (sb.version == BCACHE_SB_VERSION_CDEV
		   || sb.version == BCACHE_SB_VERSION_CDEV_WITH_UUID) {
		detail_base(devname, sb, &cd->base);
		cd->first_sector = sb.bucket_size * sb.first_bucket;
		cd->cache_sectors =
		    sb.bucket_size * (sb.nbuckets - sb.first_bucket);
		cd->total_sectors = sb.bucket_size * sb.nbuckets;
		cd->ordered = CACHE_SYNC(&sb);
		cd->discard = CACHE_DISCARD(&sb);
		cd->pos = sb.nr_this_dev;
		cd->replacement = CACHE_REPLACEMENT(&sb);
	} else {
		fprintf(stderr, "unknown bcache device type found");
		return 1;
	}
	return 0;
}

int regist(char *devname)
{
	int fd;
	fd = open("/sys/fs/bcache/register", O_WRONLY);
	if (fd < 0) {
		perror("Error opening /sys/fs/bcache/register");
		fprintf(stderr,
			"The bcache kernel module must be loaded\n");
		return 1;
	}
	if (dprintf(fd, "%s\n", devname) < 0) {
		fprintf(stderr, "Error registering %s with bcache: %m\n",
			devname);
		return 1;
	}
	close(fd);
	return 0;
}

int unregist_cset(char *cset)
{
	int fd;
	char path[100];
	sprintf(path, "/sys/fs/bcache/%s/unregister", cset);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s\n", path);
		return 1;
	}
	if (dprintf(fd, "%d\n", 1) < 0) {
		fprintf(stderr, "Failed to unregist this cache device");
		return 1;
	}

	return 0;
}

int stop_backdev(char *devname)
{
	char path[100];
	int fd;
	char buf[20];
	trim_prefix(buf, devname, 5);
	sprintf(path, "/sys/block/%s/bcache/stop", buf);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s\n", path);
		return 1;
	}
	if (dprintf(fd, "%s\n", "1") < 0) {
		fprintf(stderr, "Error stop back device %s\n", devname);
		return 1;
	}
	return 0;
}

int unregist_both(char *cset)
{
	int fd;
	char path[100];
	sprintf(path, "/sys/fs/bcache/%s/stop", cset);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s\n", path);
		return 1;
	}
	if (dprintf(fd, "%d\n", 1) < 0) {
		fprintf(stderr, "failed to stop cset and its backends %m");
		return 1;
	}
	return 0;
}

int attach(char *cset, char *devname)
{
	int fd;
	char buf[20];
	trim_prefix(buf, devname, 5);
	char path[100];
	sprintf(path, "/sys/block/%s/bcache/attach", buf);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Can't open %s:%m\n", path);
		return 1;
	}
	if (dprintf(fd, "%s\n", cset) < 0) {
		fprintf(stderr, "failed to attache to cset %s\n", cset);
		return 1;
	}

	return 0;
}

int detach(char *devname)
{
	int fd;
	char buf[20];
	trim_prefix(buf, devname, 5);
	char path[100];
	sprintf(path, "/sys/block/%s/bcache/detach", buf);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr,
			"Can't open %s,Make sure the device name is correct\n",
			path);
		return 1;
	}
	if (dprintf(fd, "%d\n", 1) < 0) {
		fprintf(stderr, "Error detach device %s:%m", devname);
		return 1;
	}
	return 0;
}

int set_backdev_cachemode(char *devname, char *cachemode)
{
	int fd;
	char path[100];
	char buf[20];
	trim_prefix(buf, devname, 5);
	sprintf(path, "/sys/block/%s/bcache/cache_mode", buf);
	fd = open(path, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr,
			"Can't open %s,Make sure the device name is correct\n",
			path);
		return 1;
	}
	if (dprintf(fd, "%s\n", cachemode) < 0) {
		printf("Failed to set cachemode for device %s:%m\n",
		       devname);
		return 1;
	}
	return 0;
}

int get_backdev_cachemode(char *devname, char *mode)
{
	int fd;
	char path[100];
	sprintf(path, "/sys/block/%s/bcache/cache_mode", devname);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("Error opening /sys/fs/bcache/register");
		fprintf(stderr,
			"The bcache kernel module must be loaded\n");
		return 1;
	}
	printf("size in func is %d", sizeof(mode));
	if (read(fd, mode, 100) < 0) {
		fprintf(stderr, "failed to fetch device cache mode\n");
		return 1;
	}

	return 0;
}
