#WHY
  Integrate multi tools to a single one.
  Display the output in a more readable way. 

#Install
  a, Inatall the dependence.
    ``` shell
	#yum install centos-release-yum4 -y
	#yum install libblkid libblkid-devel libuuid libuuid-devel libsmartcols libsmartcols-devel -y
    ```
  b, Clone the repo.
    ```shell
       #git clone https://github.com/dahefanteng/bcache-ctl.git
    ```
  c, Build 
    ```shell
	#cd bcache-ctl
        #make
    ```

#Usage:
    print 'bcache-ctl -h' to show how to use.
    the output is just like below:
    ```shell
	Usage:bcache-ctl [SUBCMD]
	show		show all bcache devices in this host
	tree		show active bcache devices in this host
	make		make regular device to bcache device
	regist 		regist device to kernel
	unregist	unregist device from kernel
	attach		attach backend device(data device) to cache device
	detach		detach backend device(data device) from cache device
	set-cachemode	set cachemode for backend device
    ```
