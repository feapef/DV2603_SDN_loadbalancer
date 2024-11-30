#!/bin/bash
IMAGE_SERVER=nginx_web
CMD="$1"
COUNT=$2
if [ $# -eq 2 ] ; then 
    case "$CMD" in
        up) 
        while [[ $COUNT -ge 1 ]]; do
            port=$((8080 + COUNT))
            docker run -d -p $port:80 --name="webserver-instance-$COUNT" -e HOST="webserver-instance-$COUNT" --hostname="webserver-instance-$COUNT" $IMAGE_SERVER
            COUNT=$((COUNT - 1))
        done
        ;;
        rm) 
        while [[ $COUNT -ge 1 ]]; do
            port=$((8080 + COUNT))
            docker stop webserver-instance-$COUNT
            docker rm webserver-instance-$COUNT
            COUNT=$((COUNT - 1))
        done
        ;;
        *) echo "error : bad cmd"
        ;;
    esac
else
    echo "error : bad usage "
fi
