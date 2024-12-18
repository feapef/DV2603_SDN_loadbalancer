#!/bin/bash

### VARIABLES
ACCESS_LOG="/var/log/nginx/access.log"

INTERFACE=eth0
HOSTNAME=webserver
IP_ADDRESS=$(ip -4 addr show $INTERFACE| grep -oP '(?<=inet\s)\d+(\.\d+){3}')
export HOSTNAME
export IP_ADDRESS

CONTROL_PLANE_IP=$CONTROL_PLANE_IP
CONTROL_PLANE_PORT=$CONTROL_PLANE_PORT

### FUNCTIONS
send_message (){
	echo $1 '->' $CONTROL_PLANE_IP:$CONTROL_PLANE_PORT
    echo $1 | netcat $CONTROL_PLANE_IP $CONTROL_PLANE_PORT
}

### TRAP
# Catch docker container stop to execute a last command
trap 'send_message "STOP $IP_ADDRESS";exit 1' SIGTERM


### COMMANDS

## Modify the webhtml page
for f in /opt/html/*; do envsubst < $f > $f-tmp ; mv $f-tmp $f ; done

## NGINX INIT
# start nginx in daemon mode
nginx 
# nginx-debug for debugging

## CONTROL PLANE
# send init message
# retry until it's not connected
CPU_USAGE="$(ps -eo %cpu --no-headers | awk '{s+=$1} END {print s}')"
while ! send_message "NEW $IP_ADDRESS $CPU_USAGE" ; do 
    sleep 1
done

while true; do
		sleep 10 #sleep 10 second
		#TRAFFIC="$(wc -l < $ACCESS_LOG)"
        CPU_USAGE="$(ps -eo %cpu --no-headers | awk '{s+=$1} END {print s}')"
		# send traffic data
		send_message "CHECK $IP_ADDRESS $CPU_USAGE"
done

# send kill message
send_message "STOP $IP_ADDRESS $TRAFFIC"
