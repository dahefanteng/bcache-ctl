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
int list_bdevs(char devs[][50]){
//	char devs[10][20]; //TODO:prevent memory leak,for now you show set the size large enough.
//	maybe we can use linked list
	printf("hello\n");
	int i = 0;
	DIR *dir;
	struct dirent *ptr;
        blkid_probe pr;
        struct cache_sb sb;
	char uuid[40];
	char set_uuid[40];
	dir = opendir("/sys/block");
	while((ptr = readdir(dir))!=NULL){
	    if (strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0){
		continue;
	    }
            char dev[20];
    	    sprintf(dev,"/dev/%s",ptr->d_name);
	//	printf("%s\n",dev);
//	    char dev[20] = "/dev/";
//		strcat(dev,ptr->d_name);
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

                uuid_unparse(sb.uuid, uuid);
                uuid_unparse(sb.set_uuid, set_uuid);
//                printf(" UUID=%s DEV=%s  SET_UUID=%s\n", uuid,dev,set_uuid);
//		printf("i is %d",i);
		strcpy(devs[i],dev);
	//	devs[i]=dev;
		i++;
//		printf("sb.csum:%s\n",uuid);
//		printf(dev,sb.csum);

	}
	strcpy(devs[i],"\0");
	closedir(dir);
	return 0;
}
*/

/*
 *
 * utils function
 */

void trim_prefix(char *dest,char *src){
	//TODU:we should not trim prefix using number;
	strcpy(dest,src+5);	
}

void get_tail(char *dest,char *src,int n){
        int num,i;
        num = strlen(src);
//	printf("\n string len is%d\n",num);
        for (i=0;i<n;i++){
//		printf("iiiii:%d,%c\n",i,src[num-n+i]);
                dest[i]=src[num-n+i];
        }
	dest[i] = '\0';
//	printf("\ni is %d, uuid:%s,len is %d",i,dest,strlen(dest));
}

void trim_tail(char *dest,char *src,int n){
	

	int num,i;
	num = strlen(src);

	for (i=0;i<num-n;i++){
		dest[i]=src[i];
//		printf("i is %d,src is %c\t %d\n",i,dest[i],dest[i]);
	}
	dest[i] = '\0';
//		printf("i is %d,src is %c\t %d\n",i,dest[i],dest[i]);
}

int get_state(struct dev *dev,char *state){
        if (dev->version == BCACHE_SB_VERSION_CDEV || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID){
                return get_cachedev_state(dev->cset,state);
        }else if(dev->version == BCACHE_SB_VERSION_BDEV || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET ){
                return get_backdev_state(dev->name,state);
        }
}


int get_backdev_state(char *devname,char *state){
    FILE* fd;
    char path[100];
    char buf[20];
    trim_prefix(buf,devname);
    sprintf(path,"/sys/block/%s/bcache/state",buf);
    fd = fopen(path,"r");
    if (fd == NULL)
    {
	strcpy(state,"inactive");
	return 0;
    }
	int i=0;
        while((state[i]=getc(fd)) != '\n' ){
	    i++;
	}
	state[i]='\0';
//	num = fread(state,1,8,fd);
//printf("strlen is %d",strlen(state));
    fclose(fd);
    return 0;
}

int get_cachedev_state(char *cset_id,char *state){
	int fd;
	char path[100];
	sprintf(path,"/sys/fs/bcache/%s/unregister",cset_id);
        fd = open(path,O_WRONLY);
        if (fd < 0)
        {
	    strcpy(state,"inactive");
        }else{
	    strcpy(state,"active");
	}
            return 0;
}

int get_bname(struct dev *dev,char *bname){
        if (dev->version == BCACHE_SB_VERSION_CDEV || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID){
                strcpy(bname,"N/A");
        }else if(dev->version == BCACHE_SB_VERSION_BDEV || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET ){
                return get_dev_bname(dev->name,bname);
        }
        return 0;
}

int get_dev_bname(char *devname,char *bname){
    int rt;
    char path[100];
    char buf[20];
    char link[100];
    char buf1[100];
    trim_prefix(buf,devname);
    sprintf(path,"/sys/block/%s/bcache/dev",buf);
    rt = readlink(path,link,sizeof(link));
    if (rt<0){
	strcpy(bname,"non-exsit");
    }else{
	//printf("rt is %d,strlen is %d",rt,strlen(link));
	trim_tail(buf1,link,strlen(link)-rt);
//	printf("bbg%s",buf);
	strcpy(bname,buf1+41);
    }
    return 0;
}

int get_point(struct dev *dev,char *point){
        if (dev->version == BCACHE_SB_VERSION_CDEV || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID){
                strcpy(point,"N/A");
        }else if(dev->version == BCACHE_SB_VERSION_BDEV || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET ){
                return get_backdev_attachpoint(dev->name,point);
        }
        return 0;
}

int get_backdev_attachpoint(char *devname,char *point){
    int rt;
    char path[100];
    char buf[20];
    char link[100];
    char uuid[40];
    char buf1[100];
    trim_prefix(buf,devname);
    sprintf(path,"/sys/block/%s/bcache/cache",buf);
    rt = readlink(path,link,sizeof(link));
    if (rt<0){
	strcpy(point,"alone");
    }else{
	trim_tail(buf1,link,strlen(link)-rt);
	get_tail(uuid,buf1,36);
	strcpy(point,uuid);
    }
    return 0;
}





int list_bdevs(struct dev **devs){
	DIR *dir;
	struct dirent *ptr;
        blkid_probe pr;
        struct cache_sb sb;
	dir = opendir("/sys/block");
	while((ptr = readdir(dir))!=NULL){
	    if (strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0){
		continue;
	    }
//	    if (strncmp(ptr->d_name,"bcache",5) == 0){
//		printf("skip bcache");
//		continue;
//	    }
            char dev[20];
    	    sprintf(dev,"/dev/%s",ptr->d_name);
//	    char dev[20] = "/dev/";
//		strcat(dev,ptr->d_name);
	        int fd = open(dev, O_RDONLY);
                if (fd == -1)
                        continue;
		//printf("%s\n",dev);

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
		

		struct dev *tmp,*current;
		int rt;
		if (*devs){
			tmp = (struct dev *)malloc(DEVLEN);
			rt = detail_base(dev,sb,tmp);
			if (rt!= 0){
				printf("error occur");
				return 1;
			}else{
				current->next=tmp;
				current = tmp;
			}
		}else{
//			printf("enter firest\n");
			tmp = (struct dev *)malloc(DEVLEN);
			rt=detail_base(dev,sb,tmp);
			if (rt!=0){
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

/*
struct dev{
    char *magic;
    uint64_t first_sector;
    uint64_t csum;
    int version;
    char label[SB_LABEL_SIZE + 1];
    char uuid[32];
    int sectors_per_block;
    int sectors_per_bucket;
    char cset[32];
};

struct bdev{
    struct dev base;
    int first_sector;
    int cache_mode;
    int cache_state;
};

//typedef int bool;
struct cdev{
    struct dev base;
    int first_sector;
    int cache_sectors;
    int total_sectors;
    bool ordered;
    bool discard;
    int pos;
    int replacement; 
};
*/

int detail_bdev(char *devname,struct bdev *bd){
//TODU: merge the detail_bdev and detail_cdev function
        struct cache_sb sb;
	uint64_t expected_csum;
        int fd = open(devname, O_RDONLY);
        if (fd < 0) {
                fprintf(stderr,"Can't open dev %s,in function:detail_bdev\n", devname);
                return 1;
        }

        if (pread(fd, &sb, sizeof(sb), SB_START) != sizeof(sb)) {
                fprintf(stderr, "Couldn't read\n");
                return 1;
        }
	
	if (!memcmp(sb.magic, bcache_magic, 16)) {
                bd->base.magic="ok";
        } else {
		bd->base.magic="bad magic";
                return 1;
        }

	if (sb.offset == SB_SECTOR) {
		bd->base.first_sector=SB_SECTOR;
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

	bd->base.version=sb.version;
	
        strncpy(bd->base.label,(char*)sb.label,SB_LABEL_SIZE);
	bd->base.label[SB_LABEL_SIZE] = '\0';
	if (*bd->base.label)
	    printf("a");
	else
	    strncpy(bd->base.label,"(empty)",7);

	uuid_unparse(sb.uuid, bd->base.uuid);

	bd->base.sectors_per_block = sb.block_size;
	bd->base.sectors_per_bucket = sb.bucket_size;
	close(fd);
        return 0;	
}

int detail_base(char *devname,struct cache_sb sb,struct dev *base){
//TODU: should we add static to return value?
	int ret;
	uint64_t expected_csum;
	strcpy(base->name,devname);
	base->magic="ok";
	base->first_sector=SB_SECTOR;
	base->csum = sb.csum;
	base->version=sb.version;
	
        strncpy(base->label,(char*)sb.label,SB_LABEL_SIZE);
	base->label[SB_LABEL_SIZE] = '\0';
	if (*base->label)
	    printf("a");
	else
	    strncpy(base->label,"(empty)",sizeof(base->label));

	uuid_unparse(sb.uuid,base->uuid);
	uuid_unparse(sb.set_uuid, base->cset);
	base->sectors_per_block = sb.block_size;
	base->sectors_per_bucket = sb.bucket_size;
	if ((ret = get_state(base,base->state)) !=0){
		printf("err when get state");
		return ret;
	}
	if ((ret = get_bname(base,base->bname)) !=0){
		printf("err when get bname");
		return ret;
	}
	if ((ret = get_point(base,base->attachuuid)) !=0){
		printf("err when get attach");
		return ret;
	}
//	printf("attach uuid is %s\n",base->attachuuid);
        return 0;	
}

int detail_dev(char *devname,struct bdev *bd,struct cdev *cd,int *type){
//TODU: merge the detail_bdev and detail_cdev function
        struct cache_sb sb;
	uint64_t expected_csum;
        int fd = open(devname, O_RDONLY);
        if (fd < 0) {
		printf("\nstring len is %d\n",strlen(devname));
                printf("Can't open dev  %s,in function detail_dev,fd is %d\n", devname,fd);
                return 1;
        }

        if (pread(fd, &sb, sizeof(sb), SB_START) != sizeof(sb)) {
                fprintf(stderr, "Couldn't read\n");
                return 1;
        }
	
	if (memcmp(sb.magic, bcache_magic, 16)) {
                return 1;
        }

	if (!sb.offset == SB_SECTOR) {
                fprintf(stderr, "Invalid superblock (bad sector)\n");
                return 1;
        }
	
	expected_csum = csum_set(&sb);
        if (!sb.csum == expected_csum) {
                return 1;
        }	

	*type = sb.version;
	if (sb.version == BCACHE_SB_VERSION_BDEV) {
		detail_base(devname,sb,&bd->base);
		bd->first_sector=BDEV_DATA_START_DEFAULT;
		bd->cache_mode=BDEV_CACHE_MODE(&sb);
		bd->cache_state=BDEV_STATE(&sb);
	}else if (sb.version == BCACHE_SB_VERSION_CDEV || sb.version == BCACHE_SB_VERSION_CDEV_WITH_UUID) {
		detail_base(devname,sb,&cd->base);
		cd->first_sector=sb.bucket_size * sb.first_bucket;
		cd->cache_sectors=sb.bucket_size * (sb.nbuckets - sb.first_bucket);
		cd->total_sectors=sb.bucket_size * sb.nbuckets;
		cd->ordered=CACHE_SYNC(&sb);
		cd->discard=CACHE_DISCARD(&sb);
		cd->pos=sb.nr_this_dev;
		cd->replacement=CACHE_REPLACEMENT(&sb);
	}else {
		fprintf(stderr,"unknown bcache device type found");
		return 1;
	}	
        return 0;	
}

int registe(char *devname){
   // char *devname = "/dev/sdi";
    int fd;
    fd = open("/sys/fs/bcache/register", O_WRONLY);
    if (fd < 0)
    {
        perror("Error opening /sys/fs/bcache/register");
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%s\n", devname) < 0)
    {
        fprintf(stderr, "Error registering %s with bcache: %m\n", devname);
        return 1;
    }
    close(fd);
    return 0;
}

int unregiste_cset(char *cset){
//    char *devname = "/dev/sdi";
    int fd;
    char path[100];
    sprintf(path,"/sys/fs/bcache/%s/unregister",cset);
    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%d\n", 1) < 0)
    {
        return 1;
    }

    return 0;
}

int stop_backdev(char *devname){
    char path[100];
    int fd;
    char buf[20];
    trim_prefix(buf,devname);
    sprintf(path,"/sys/block/%s/bcache/stop",buf);
    fd = open(path,O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%s\n", "1") < 0)
    {
    //    fprintf(stderr, "Error stop back device %s\n", devname);
        return 1;
    }
    return 0;
}

int unregiste_both(char *cset){
    int fd;
    char path[100];
    sprintf(path,"/sys/fs/bcache/%s/stop",cset);
    fd = open(path, O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%d\n", 1) < 0)
    {
        return 1;
    }
    return 0;
}

int attach(char *cset,char *devname){
    int fd;
    char buf[20];
    trim_prefix(buf,devname);
    char path[100];
    sprintf(path,"/sys/block/%s/bcache/attach",buf);
    fd = open(path,O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%s\n", cset) < 0)
    {
        return 1;
    }

    return 0;
}

int detach(char *devname){
    int fd;
    char buf[20];
    trim_prefix(buf,devname);
    char path[100];
    sprintf(path,"/sys/block/%s/bcache/detach",buf);
    fd = open(path,O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%d\n", 1) < 0)
    {
        return 1;
    }
    return 0;
}

int set_backdev_cachemode(char *devname,char *cachemode){
    int fd;
    char path[100];
    char buf[20];
    trim_prefix(buf,devname);
    sprintf(path,"/sys/block/%s/bcache/cache_mode",buf);
    fd = open(path,O_WRONLY);
    if (fd < 0)
    {
        fprintf(stderr, "The bcache kernel module must be loaded\n");
        return 1;
    }
    if (dprintf(fd, "%s\n", cachemode) < 0)
    {
        return 1;
    }
    return 0;
}

int get_backdev_cachemode(char *devname,char *mode){
    int fd;
    char path[100];
    sprintf(path,"/sys/block/%s/bcache/cache_mode",devname);
    fd = open(path,O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening /sys/fs/bcache/register");
        fprintf(stderr, "The bcache kernel module must be loaded\n");
	return 1;
    }
    printf("size in func is %d",sizeof(mode));
    if (read(fd,mode,100) < 0)
    {
        fprintf(stderr, "failed to fetch device cache mode\n");
	return 1;
    }
    
    return 0;
}





/*
int main(){
//	list_bcacheset();
	//unregiste_cset("112edd51-a548-4945-b0eb-3df948e90fda");
//	stop_backdev("sdh");
//	unregiste_both("112edd51-a548-4945-b0eb-3df948e90fda");
//	detach("sdh");
	//attach("112edd51-a548-4945-b0eb-3df948e90fda","sdh");
	//int rt;
	//char mode[42];
	//rt = get_backdev_state("sdh",mode);

listdevstest
	char a[500][50];	
	list_bdevs(a);

 
 	printf("\n%d\n",sizeof(a));
	int i;
	for (i=0;i<5;i++){
	int r;
	r = strcmp(a[i],"\0");
	printf("r is %d",r);
	if  (strcmp(a[i],"\0")==0){
		printf("empty in %d",i);
		break;
	}
	printf("%d %s \n",i,a[i]);
}

	struct bdev back;
	printf("init is %d\n",back.first_sector);
	int rt;
        rt = detail_bdev("/dev/sdh",&back);
	printf("result code is %d,back is %d\n,base is %d\n",rt,(&back)->first_sector,(&back)->base.first_sector);
}
*/


