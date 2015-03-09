#!/bin/sh

node=$1

if [ "$node" != "" ]; then
	echo
else
	echo
	echo "Необходим 1 параметр: ./install.sh ip_address"
	exit
fi

pthe=/etc
pthec=/etc/cron.d
pthu=/home/utils

ftpfile=ftp.txt
ftplog=ftp.log

eth0_ip="192.168.1.127"
eth0_netmask="255.255.255.0"

eth1_ip="192.168.3.130"
eth1_netmask="255.255.255.0"

rs_mode="0"

echo "Настройка интерфейса eth0:"
read -p "address ($eth0_ip): " iip
if [ "$iip" != "" ]; then
	eth0_ip=$iip
fi

read -p "netmask ($eth0_netmask): " inetmask
if [ "$inetmask" != "" ]; then
	eth0_netmask=$inetmask
fi

echo
echo "Настройка интерфейса eth1:"
read -p "address ($eth1_ip): " iip1
if [ "$iip1" != "" ]; then
	eth1_ip=$iip1
fi

read -p "netmask ($eth1_netmask): " inetmask1
if [ "$inetmask1" != "" ]; then
	eth1_netmask=$inetmask1
fi

echo
echo "Установка интерфейса серийных портов:"
echo "0 - set to RS232"
echo "1 - set to RS485-2Wires"
echo "2 - set to RS422"
echo "3 - set to RS485-4Wires"
read -p "RS mode ($rs_mode): " irs_mode
if [ "$irs_mode" != "" ]; then
	rs_mode=$irs_mode
fi

# Генерация файла 'rc'
sh rc.in $eth0_ip $eth0_netmask $eth1_ip $eth1_netmask $rs_mode

echo "ascii" > $ftpfile

echo "mkdir /home/utils" >> $ftpfile
echo "chmod 755 /home/utils" >> $ftpfile
echo "mkdir /home/tabr" >> $ftpfile
echo "chmod 755 /home/tabr" >> $ftpfile
echo "mkdir /home/slave" >> $ftpfile
echo "chmod 755 /home/slave" >> $ftpfile

echo "cd $pthe" >> $ftpfile
echo "put rc" >> $ftpfile
echo "chmod 755 rc" >> $ftpfile

echo "cd $pthec" >> $ftpfile
echo "put crontab" >> $ftpfile
echo "chmod 644 crontab" >> $ftpfile

echo "cd $pthu" >> $ftpfile
echo "put upramdisk2" >> $ftpfile
echo "chmod 755 upramdisk2" >> $ftpfile
echo "put cron_wdt" >> $ftpfile
echo "chmod 755 cron_wdt" >> $ftpfile
echo "exit" >> $ftpfile
chmod 640 $ftpfile

ftp $node < $ftpfile > ftplog
echo "MOXA RC files udated ($node) ... ok"
