#!/bin/sh

sudo cp ./99-dim.rules  /etc/udev/rules.d
sudo cp ./99-usb-hub-rpi4.rules /etc/udev/rules.d

echo "Restarting udev"
echo ""
sudo service udev reload
sudo service udev restart
echo "finish "
