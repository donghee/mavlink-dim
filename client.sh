ps aux | grep mavlink_dim | awk '{ print $2 }' | xargs -n 1 sudo kill -9

make clean
make mavlink_dim_client

# linux pc
./tools/reset-usbhub.sh

./mavlink_dim_client 223.171.56.36 14450 9120
