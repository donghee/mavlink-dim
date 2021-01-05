reset

scp pi@raspberrypi.local:/home/pi/src/mavlink_dim/src/*  src/
scp pi@raspberrypi.local:/home/pi/src/mavlink_dim/kse_dim/*  kse_dim/
ps aux | grep mavlink_dim | awk '{ print $2 }' | xargs -n 1 sudo kill -9
make clean
make
sudo ./mavlink_dim_client 10.243.45.201

reset
