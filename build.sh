#!/bin/sh
# Tested on Ubuntu 18.04
sudo apt-get install libdbus-1-dev libglib2.0-dev libsdl1.2-dev -y # install dependencies
wget http://www.kernel.org/pub/linux/bluetooth/bluez-4.101.tar.xz  # download bluez-4.101
tar -xf bluez-4.101.tar.xz
cp bluez-plugin/wmemu.c bluez-4.101/plugins/                       # copy files to plugins dir
cp bluez-plugin/Makefile bluez-4.101/plugins/
sudo apt purge bluez                                               # remove existing bluez
cd bluez-4.101/ && ./configure --enable-wiimote
make && sudo make install                                          # install bluez-4.101
cd plugins/ && make                                                # make the plugin
cd .. && cd .. && make                                             # make the wiimoteemu
echo move /bluez-4.101/plugin/wmemu.so to /usr/local/lib/bluetooth/plugins or equivalent

