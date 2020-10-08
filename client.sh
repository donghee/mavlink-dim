
ps aux | grep mavlink_dim | awk '{print $2}' | xargs -n 1 kill -9

./mavlink_dim_client
