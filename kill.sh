ps aux | grep mavlink_dim | awk '{ print $2 }' | xargs -n 1 sudo kill -9
