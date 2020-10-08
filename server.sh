reset
#cd ~/src/dim/; scp -r donghee@192.168.88.18:/home/donghee/src/dim/mavlink_dim ./ ; cd ~/src/dim/mavlink_dim; sudo ./mavlink_dim_server
scp -r donghee@192.168.88.18:/home/donghee/src/dim/mavlink_dim/mavlink_dim_server ./mavlink_dim_server
sudo ps aux |grep mavlink_dim_server | awk '{print }' | sudo xargs kill -9
sudo ./mavlink_dim_server
