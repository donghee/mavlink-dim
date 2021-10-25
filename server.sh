ps aux |grep mavlink_dim_server |grep -v 'watch' | awk '{ print $2 }' | sudo xargs sudo kill -9

make clean
make mavlink_dim_server

# raspberry pi 4
./tools/reset-usbhub-rpi4.sh

./mavlink_dim_server  /dev/ttyS0 921600
