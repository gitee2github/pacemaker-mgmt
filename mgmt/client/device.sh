#!/bin/bash

LANG=C
alldevice=`fdisk -l | grep "Disk /dev" |awk -F ':' '{print $1}'|awk '{print $2}'`
allpart=`fdisk -l |grep '^/dev'| awk -F ' ' '{print $1}'`
use_part=`fdisk -l |grep '^/dev'|egrep  'swap|Extended|\*' |  awk -F ' ' '{print $1}'`
mount_part=`df |awk -F " " '{print $1}'|grep /dev`
pv_device=`pvscan  2>/dev/null | grep "PV" | awk -F ' ' '{print $2}'`

# alldevice - device with part + allpart - use_part - mount_part - pv_device
for device in $alldevice
do
	echo $allpart | grep -q $device 
	if [ $? != 0 ]; then
		echo $pv_device | grep -q $device
		if [ $? != 0 ]; then
			echo $mount_part | grep -q $device
			if [ $? != 0 ]; then
				echo $device
			fi
		fi
	fi
done
	
for part in $allpart
do
	echo $use_part | grep -q $part
	if [ $? != 0 ]; then
		echo $mount_part | grep -q $part
		if [ $? != 0 ]; then
			echo $pv_device | grep -q $part
			if [ $? != 0 ]; then
				echo $part
			fi
		fi
	fi
done
