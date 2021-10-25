ps aux | grep mavlink_dim | awk '{ print $2 }' | xargs -n 1 sudo kill -9
make clean
make
./tools/reset-usbhub.sh
./mavlink_dim_client 223.171.56.36
