sudo useradd -r -U -s /usr/sbin/nologin ping
sudo setcap cap_net_raw+ep ./ft_ping
# sudo setcap cap_net_raw+ep /bin/ping # Optional: set the capability on the system's ping command
sudo chown root:ping ./ft_ping
sudo chmod 750 ./ft_ping
sudo chmod u+s ./ft_ping