# WHY 
Integrate multi tools to a single one. Display the output in a more readable way.

# Install 
- 1. Inatall the dependence. 
    For Centos:
    ```shell
	#yum install centos-release-yum4 -y 
	#yum install libblkid libblkid-devel libuuid libuuid-devel libsmartcols libsmartcols-devel -y
    ```
    For OpenSUSE:
    ```shell
	#zypper install libblkid libblkid-devel libuuid libuuid-devel libsmartcols libsmartcols-devel 
    ```
- 2. Clone the repo.
    ```shell 
	#git clone https://github.com/dahefanteng/bcache-ctl.git 
    ```
- 3. Build
    ```shell 
	#cd bcache-ctl 
	#make
    ```

# Usage: 
    'bcache-ctl --help'
      
    Usage:bcache-ctl [SUBCMD]
	show		show all bcache devices in this host
	tree		show active bcache devices in this host
	make		make regular device to bcache device
	regist 		regist device to kernel
	unregist	unregist device from kernel
	attach		attach backend device(data device) to cache device
	detach		detach backend device(data device) from cache device
	set-cachemode	set cachemode for backend device
