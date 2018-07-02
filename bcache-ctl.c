#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "make-bcache.h"
#include "bcache.h"
#include "lib.h"

void myusage()
{
	printf("error using bcache-mgr");
}

/*
int show_bdevs(){
        char a[500][50];        
        list_bdevs(a);
 
        printf("\n%d\n",sizeof(a));
        int i;
        for (i=0;i<500;i++){
        int r;
       // r = strcmp(a[i],"\0");
       // printf("r is %d",r);
        if  (strcmp(a[i],"\0")==0){
                 printf("empty in %d",i);
                break;
        }
        printf("%d %s \n",i,a[i]);
	}
	return 0;
}
*/

int get_state(struct dev *dev,char *state){
	if (dev->version == BCACHE_SB_VERSION_CDEV || dev->version == BCACHE_SB_VERSION_CDEV_WITH_UUID){
		return get_cachedev_state(dev->name,state); 	
	}else if(dev->version == BCACHE_SB_VERSION_BDEV || dev->version == BCACHE_SB_VERSION_BDEV_WITH_OFFSET ){
		return get_backdev_state(dev->name,state);
	}
}

int show_bdevs(){
	struct dev *devs=NULL;
	struct dev *tmp;
	int rt;
        rt =list_bdevs(&devs);
	if (rt != 0){
		printf("result is non-zero:%d",rt);
		return rt;
	}
		
	printf("name\t\tuuid\t\t\t\t\tcset_uuid\t\t\t\ttype\tstate\n");
	while (devs) {
		printf("%s\t%s\t%s\t%d",devs->name,devs->uuid,devs->cset,devs->version); 
		char state[20];
		get_state(devs,state);
		printf("\t%s",state);
		putchar('\n');
		tmp = devs;
		devs = devs->next;
		free(tmp);
	}
	return 0;
}

int detail(char *devname){
	printf("enter detail");
	struct bdev bd;
	struct cdev cd;
	int type =1;
	int rt;
	rt = detail_dev(devname,&bd,&cd,&type);
	if (rt != 0) {
		printf("err occur ,rt is %d",rt);
		return rt;
	}
	if (type==BCACHE_SB_VERSION_BDEV) {
		printf("sb.magic\t\t%s\n",bd.base.magic);
		printf("sb.first_sector\t\t%" PRIu64 "\n", bd.base.first_sector);
		printf("sb.csum\t\t\t%" PRIX64 "\n", bd.base.csum);
		printf("sb.version\t\t%" PRIu64 "\n", bd.base.version);
		putchar('\n');
		printf("dev.label\t\t%s\n",bd.base.label);
		printf("dev.uuid\t\t%s\n", bd.base.uuid);
		printf("dev.sectors_per_block\t%u\n"
               		"dev.sectors_per_bucket\t%u\n",
               		bd.base.sectors_per_block,
               		bd.base.sectors_per_bucket);
		printf("dev.data.first_sector\t%ju\n"
                       "dev.data.cache_mode\t%ju\n",
                       bd.first_sector,
                       bd.cache_mode);
		printf("dev.data.cache_state\t%ju\n",
                       bd.cache_state);
		putchar('\n');
		printf("cset.uuid\t\t%s\n", bd.base.cset);
	}else if (type ==BCACHE_SB_VERSION_CDEV || type == BCACHE_SB_VERSION_CDEV_WITH_UUID) {
		printf("enter cache ok\n");
		printf("that is %s",cd.base.uuid);
		printf("sb.magic\t\t%s\n",cd.base.magic);
		printf("sb.first_sector\t\t%" PRIu64 "\n", cd.base.first_sector);
		printf("sb.csum\t\t\t%" PRIX64 "\n", cd.base.csum);
		printf("sb.version\t\t%" PRIu64 , cd.base.version);
		printf(" [cache device]\n");
		putchar('\n');
		printf("dev.label\t\t%s\n",cd.base.label);
		printf("dev.uuid\t\t%s\n", cd.base.uuid);
		printf("dev.sectors_per_block\t%u\n"
               		"dev.sectors_per_bucket\t%u\n",
               		cd.base.sectors_per_block,
               		cd.base.sectors_per_bucket);
		printf("dev.cache.first_sector\t%u\n"
                       "dev.cache.cache_sectors\t%ju\n"
                       "dev.cache.total_sectors\t%ju\n"
                       "dev.cache.ordered\t%s\n"
                       "dev.cache.discard\t%s\n"
                       "dev.cache.pos\t\t%u\n"
                       "dev.cache.replacement\t%d",
                       cd.first_sector,
                       cd.cache_sectors,
                       cd.total_sectors,
                       cd.ordered ? "yes":"no",
                       cd.discard ? "yes":"no",
                       cd.pos,
                       cd.replacement);
		 switch (cd.replacement) {
                        case CACHE_REPLACEMENT_LRU:
                                printf(" [lru]\n");
                                break;
                        case CACHE_REPLACEMENT_FIFO:
                                printf(" [fifo]\n");
                                break;
                        case CACHE_REPLACEMENT_RANDOM:
                                printf(" [random]\n");
                                break;
                        default:
                                putchar('\n');
                }

		putchar('\n');
		printf("cset.uuid\t\t%s\n", cd.base.cset);
	}else{
		printf("type is %d",type);
	}
}

int main(int argc, char **argv) 
{
	char *subcmd;
	if (argc < 2){
	    myusage();
	    return 1;
  	}else{
		subcmd = argv[1];
		argc--;
		argv=&argv[1];
	}

	if (strcmp(subcmd,"make-bcache")==0) {
		make_bcache(argc,argv);

	} else if(strcmp(subcmd,"show")==0){
		printf("enter show");
		if (argc == 1){
			return show_bdevs();
		}else{
			detail(argv[1]);
		}
	} else if(strcmp(subcmd,"registe")==0){
		printf("enter registe");
		return registe(argv[1]);
	} else if(strcmp(subcmd,"unregiste")==0){
		printf("enter unregiste");
		struct bdev bd;
		struct cdev cd;
		int type =1;
		int rt;
		rt = detail_dev(argv[1],&bd,&cd,&type);
		if (rt != 0) {
			printf("err occur ,rt is %d",rt);
			return rt;
		}
		if (type==BCACHE_SB_VERSION_BDEV) {
			return stop_backdev(argv[1]);	
		}else if (type ==BCACHE_SB_VERSION_CDEV || type == BCACHE_SB_VERSION_CDEV_WITH_UUID){
			return unregiste_cset(cd.base.cset);
		}
		return 1;
	} else if(strcmp(subcmd,"attach")==0){
		printf("enter attach");
		return attach(argv[1],argv[2]);
	} else if(strcmp(subcmd,"detach")==0){
		printf("enter detach");
		return detach(argv[1]);
	}else{
		printf("abc");
		myusage();
	}

	return 0;
}

