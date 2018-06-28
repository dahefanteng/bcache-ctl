struct dev{
    char *magic;
    uint64_t first_sector;
    uint64_t csum;
    int version;
    char label[SB_LABEL_SIZE + 1];
    char uuid[40];
    int sectors_per_block;
    int sectors_per_bucket;
    char cset[40];
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
    unsigned int replacement;
};


int list_bdevs(char devs[][50]);
int detail_dev(char *devname,struct bdev *bd,struct cdev *cd,int *type);
int registe(char *devname);
int stop_backdev(char *devname);
int unregiste_cset(char *cset);
int attach(char *cset,char *devname);
int detach(char *devname);
