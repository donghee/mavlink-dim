#reset
#cd ~/src/dim/; scp -r donghee@192.168.88.18:/home/donghee/src/dim/mavlink_dim ./ ; cd ~/src/dim/mavlink_dim; sudo ./mavlink_dim_server
#scp -r donghee@192.168.88.18:/home/donghee/src/dim/mavlink_dim/mavlink_dim_server ./mavlink_dim_server
sudo ps aux |grep mavlink_dim_server |grep -v 'watch' | awk '{ print $2 }' | sudo xargs sudo kill -9
sudo tools/uhubctl/uhubctl -l 1-1 -p 1-4 -a off
sleep 2
sudo tools/uhubctl/uhubctl -l 1-1 -p 1-4 -a on
sleep 2
make
#sudo ./mavlink_dim_server  /dev/ttyTHS1
sudo ./mavlink_dim_server  /dev/ttyS0 921600

#reset
