#check plugins
#tshark -G folders | grep ^Global.Lua.Plugins | cut -f2
sudo tshark -i lo  -T text -f "udp port 14550 or udp port 14551" -X lua_script:/home/donghee/.wireshark/plugins/mavlink_1_common.lua -O "mavlink_proto"
