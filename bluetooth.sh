#!/bin/bash

sudo apt-get install -y git bc libusb-dev libdbus-1-dev libglib2.0-dev libudev-dev libical-dev libreadline-dev libbluetooth-dev autoconf bluetooth
wget https://s3.amazonaws.com/json-c_releases/releases/json-c-0.13.tar.gz
tar -xvf json-c-0.13.tar.gz 
cd json-c-0.13/ 
./configure --prefix=/usr --disable-static && make 
sudo make install
cd ..
wget https://mirrors.edge.kernel.org/pub/linux/libs/ell/ell-0.6.tar.xz 
tar -xvf ell-0.6.tar.xz 
cd ell-0.6/ 
sudo ./configure --prefix=/usr 
sudo make 
sudo make install
cd ..
wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.50.tar.xz 
tar -xvf bluez-5.50.tar.xz 
cd bluez-5.50/
./configure --enable-mesh --prefix=/usr --mandir=/usr/share/man --sysconfdir=/etc --localstatedir=/var
make
sudo make install
cd ..
sudo cp /usr/lib/bluetooth/bluetoothd /usr/lib/bluetooth/bluetoothd-543.orig
sudo ln -sf /usr/libexec/bluetooth/bluetoothd /usr/lib/bluetooth/bluetoothd 
sudo systemctl daemon-reload
rm *.tar.* 
bluetoothd -v 
meshctl -v
