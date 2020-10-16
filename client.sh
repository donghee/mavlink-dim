reset

scp pi@raspberrypi.local:/home/pi/src/dim/mavlink_dim/src/*  src/
ps aux | grep mavlink_dim | awk '{ print $2 }' | xargs -n 1 sudo kill -9
make
sudo ./mavlink_dim_client

reset
