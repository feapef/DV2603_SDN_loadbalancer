#!/bin/bash

### VARIABLES
ACCESS_LOG="/var/log/nginx/access.log"

INTERFACE=eth0
IP_ADDRESS=$(ip -4 addr show $INTERFACE| grep -oP '(?<=inet\s)\d+(\.\d+){3}')

CONTROL_PLANE_IP=172.17.0.1
CONTROL_PLANE_PORT=8082

### FUNCTIONS
send_message (){
	echo $1
    echo $1 | netcat $CONTROL_PLANE_IP $CONTROL_PLANE_PORT
}

### TRAP
# Catch docker container stop to execute a last command
trap 'send_message "STOP $IP_ADDRESS";exit 1' SIGTERM


### COMMANDS

## NGINX INIT
# create the access log file to be sure it exists
touch $ACCESS_LOG
# start nginx in daemon mode
nginx 
# nginx-debug for debugging

## CONTROL PLANE
# send init message
CPU_USAGE="ps -eo %cpu --no-headers | awk '{s+=$1} END {print s}'"
send_message "NEW $IP_ADDRESS $CPU_USAGE"

while true; do
		sleep 30 #sleep 30 second
		#TRAFFIC="$(wc -l < $ACCESS_LOG)"
		CPU_USAGE="ps -eo %cpu --no-headers | awk '{s+=$1} END {print s}'"
		# send traffic data
		send_message "CHECK $IP_ADDRESS $CPU_USAGE"
done

# send kill message
send_message "STOP $IP_ADDRESS $TRAFFIC"
