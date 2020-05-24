#!/bin/bash

nodenum=`cat  /var/lib/pacemaker/cib/cib.xml | grep clone-gfs-meta_attributes-clone-max|awk '{print $4}'|awk -F \" '{print $2}'`
if [ -z "$nodenum" ];then
	nodenum=`crm_node -l | wc -l`
fi

diskpath=`cat  /var/lib/pacemaker/cib/cib.xml | grep gfs_instance_device|awk '{print $4}'|awk -F \" '{print $2}'`

#echo $diskpath
#echo $nodenum
if [ ! -z $nodenum ] && [ ! -z $diskpath ];then
	mkfs.gfs2 -p lock_dlm -j $nodenum -t cs2c:cs2cgfs $diskpath
else
	echo "get format param error!"
fi

