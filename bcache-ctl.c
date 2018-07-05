#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "make-bcache.h"
#include "bcache.h"
#include "lib.h"

int  myusage()
{
	fprintf(stderr,
		"Usage:bcache-ctl [SUBCMD]\n"
		"	show		show all bcache devices in this host\n"
		"	tree		show active bcache devices in this host\n"
		"	make-bcache	make regular device to bcache device\n"
		"	regist 		regist device to kernel\n"
		"	unregist	unregist device from kernel\n"
		"	attach		attach backend device(data device) to cache device\n"
		"	detach		detach backend device(data device) from cache device\n"
		"	set-cachemode	set cachemode for backend device\n");
	return EXIT_FAILURE;
}

int showusage(){
	fprintf(stderr,
	"Usage: show [option]"
	"	show overall information about all devices\n"
	"	-d devicename		show the detail infomation about this device\n"
	"	-o 			show overall information about all devices with detail info \n");
	return EXIT_FAILURE;
}

int registusage(){
	fprintf(stderr,
	"Usage:regist devicename	regist device as bcache device to kernel\n");
	return EXIT_FAILURE;
}

int unregistusage(){
	fprintf(stderr,
	"Usage:unregist devicename	unregist device from kernel\n");
	return EXIT_FAILURE;
}

int attachusage(){
	fprintf(stderr,
	"Usage:attach cset_uuid|cachedevice datadevice\n");
	return EXIT_FAILURE;
}

int detachusage(){
	fprintf(stderr,
	"Usage:detach devicename\n");
	return EXIT_FAILURE;
}

int setcachemodeusage(){
	fprintf(stderr,
	"Usage:set-cachemode devicename modetype\n");
	return EXIT_FAILURE;
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

void free_dev(struct dev *devs){
	struct dev *tmp;
	while (devs) {
		tmp = devs;
		devs = devs->next;
		free(tmp);	
	}
}

int show_bdevs_detail(){
	struct dev *devs=NULL;
	struct dev *prev;
	int rt;
        rt =list_bdevs(&devs);
	if (rt != 0){
		fprintf(stderr,"result  is non-zero:%d",rt);
		return rt;
	}
	prev = devs;	
	printf("Name\t\tUuid\t\t\t\t\tCset_Uuid\t\t\t\tType\t\tState\t\tBname\t\tAttachToDev\tAttachToCset\n");
		char state[20];
	while (devs) {
		printf("%s\t%s\t%s\t%d",devs->name,devs->uuid,devs->cset,devs->version); 
	        switch (devs->version) {
                // These are handled the same by the kernel
                case BCACHE_SB_VERSION_CDEV:
                case BCACHE_SB_VERSION_CDEV_WITH_UUID:
                        printf(" (cache)");
                        break;

                // The second adds data offset support
                case BCACHE_SB_VERSION_BDEV:
                case BCACHE_SB_VERSION_BDEV_WITH_OFFSET:
                        printf(" (data)");
                        break;

                default:
                        printf(" (unknown)");
                        // exit code?
                        return 0;
        	}

		printf("\t%s",devs->state);
		if (strlen(devs->state)%8 != 0){
			putchar('\t');
		}
		
		printf("\t%-16s",devs->bname);
		
		char attachdev[30];
//		printf("\n%d\n",strlen(devs->cset));
		if (strlen(devs->attachuuid) == 36){
			cset_to_devname(devs,devs->cset,attachdev);
		}else{
			strcpy(attachdev,"N/A");
		}
		printf("%-16s",attachdev);

		//TODU:adjust the distance between two lines
		printf("%s",devs->attachuuid);
		putchar('\n');

		devs = devs->next;
	}
	free_dev(prev);
	return 0;
}


int show_bdevs(){
	struct dev *devs=NULL;
	struct dev *prev;
	prev=devs;
	int rt;
        rt =list_bdevs(&devs);
	if (rt != 0){
		fprintf(stderr,"result is non-zero:%d",rt);
		return rt;
	}
		
	printf("Name\t\tType\t\tState\t\tBname\t\tAttachToDev\n");
	char state[20];
	while (devs) {
		printf("%s\t%d",devs->name,devs->version); 
	        switch (devs->version) {
                // These are handled the same by the kernel
                case BCACHE_SB_VERSION_CDEV:
                case BCACHE_SB_VERSION_CDEV_WITH_UUID:
                        printf(" (cache)");
                        break;

                // The second adds data offset support
                case BCACHE_SB_VERSION_BDEV:
                case BCACHE_SB_VERSION_BDEV_WITH_OFFSET:
                        printf(" (data)");
                        break;

                default:
                        printf(" (unknown)");
                        // exit code?
                        return 0;
        }

		printf("\t%s",devs->state);
		if (strlen(devs->state)%8 != 0){
			putchar('\t');
		}
		
		printf("\t%-16s",devs->bname);

		char attachdev[30];
//		printf("\n%d\n",strlen(devs->cset));
		if (strlen(devs->attachuuid) == 36){
			cset_to_devname(devs,devs->cset,attachdev);
		}else{
			strcpy(attachdev,"N/A");
		}
		printf("%s",attachdev);
		putchar('\n');

		devs = devs->next;
	}
	free_dev(prev);
	return 0;
}

int detail(char *devname){
	struct bdev bd;
	struct cdev cd;
	int type =1;
	int rt;
	rt = detail_dev(devname,&bd,&cd,&type);
	if (rt != 0) {
		fprintf(stderr,"err occur ,rt is %d",rt);
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

int tree(){
	printf(".\n");
	struct dev *devs=NULL;
	struct dev *prev,*tmp;
	int rt;
        rt =list_bdevs(&devs);
	if (rt != 0){
		fprintf(stderr,"result is non-zero:%d",rt);
		return rt;
	}
	prev = devs;	
	tmp = devs;
	while (devs){
		if ((devs->version == BCACHE_SB_VERSION_CDEV || devs->version == BCACHE_SB_VERSION_CDEV_WITH_UUID) && strcmp(devs->state,"active") == 0 ){
			printf("%s",devs->name);
			while (tmp) {
				if (strcmp(devs->cset,tmp->attachuuid)==0){
					putchar('\n');
					putchar(124);
					putchar(45);
					putchar(45);
					putchar(45);
					printf("%s",tmp->name);
					putchar(45);
					putchar(45);
					putchar(45);
					printf("%s",tmp->bname);
				}
				tmp=tmp->next;
			}
			tmp=prev;
			putchar('\n');
		}
		devs = devs->next;
	}
	free_dev(prev);
}

int attach_both(char *cdev,char *backdev){
	struct bdev bd;
	struct cdev cd;
	int type =1;
	int rt;
	char buf[100];
	if (strlen(cdev)!=36){
		rt = detail_dev(cdev,&bd,&cd,&type);
		if (type != BCACHE_SB_VERSION_CDEV && type != BCACHE_SB_VERSION_CDEV_WITH_UUID){
			fprintf(stderr,"%s is not an cache device",cdev);
			return 1;
		}
		strcpy(buf,cd.base.cset);	
	}else{
		strcpy(buf,cdev);	
	}
	return attach(buf,backdev);
}

//TODO:check the args in the proper way
int main(int argc, char **argv) 
{
	char *subcmd;
	if (argc < 2){
	    myusage();
	    return 1;
  	}else{
		subcmd = argv[1];
		argc--;
		argv=argv+1;
	}

	if (strcmp(subcmd,"make-bcache")==0) {
		make_bcache(argc,argv);

	} else if(strcmp(subcmd,"show")==0){
//		if (argc == 1){
//			return show_bdevs();
//		}else if (argc == 2){
//			detail(argv[1]);
//		}else{
//			return showusage();
//		}
		int ret;
		int o=0;
		char *devname;
		int more =0;
		int device=0;
		
		while((o = getopt(argc,argv,"od:")) != EOF){
			switch(o){
				case 'd':
					devname=optarg;
					device=1;
					break;
				case 'o':
					more=1;
					break;
			}
		}
		if (device){
			return detail(devname);
		} else if (more){
			return show_bdevs_detail();
		}else{
			return show_bdevs();
		}
	} else if(strcmp(subcmd,"tree")==0){
		return tree();
	} else if(strcmp(subcmd,"regist")==0){
		if (argc != 2){
			return registusage();
		}
		return regist(argv[1]);
	} else if(strcmp(subcmd,"unregist")==0){
		if (argc != 2){
			return unregistusage();
		}
		struct bdev bd;
		struct cdev cd;
		int type =1;
		int rt;
		rt = detail_dev(argv[1],&bd,&cd,&type);
		if (rt != 0) {
			fprintf(stderr,"err occur ,ret is %d",rt);
			return rt;
		}
		if (type==BCACHE_SB_VERSION_BDEV) {
			return stop_backdev(argv[1]);	
		}else if (type ==BCACHE_SB_VERSION_CDEV || type == BCACHE_SB_VERSION_CDEV_WITH_UUID){
			return unregist_cset(cd.base.cset);
		}
		return 1;
	} else if(strcmp(subcmd,"attach")==0){
		if (argc != 3){
			return attachusage();
		}
		return attach_both(argv[1],argv[2]);
	} else if(strcmp(subcmd,"detach")==0){
		if (argc != 2){
			return detachusage();
		}
		return detach(argv[1]);
	}else if(strcmp(subcmd,"set-cachemode")==0){
		if (argc != 3){
			return setcachemodeusage();
		}
		return set_backdev_cachemode(argv[1],argv[2]);
	}else{
		myusage();
	}
	return 0;
}

