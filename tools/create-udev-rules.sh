#!/bin/sh

sudo cp ./dim-99.rules  /etc/udev/rules.d
echo "Restarting udev"
echo ""
sudo service udev reload
sudo service udev restart
echo "finish "
