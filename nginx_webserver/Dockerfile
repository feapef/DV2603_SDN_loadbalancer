FROM nginx
RUN apt-get update \
    && apt-get install -y netcat-openbsd\
    && apt-get install -y iproute2 \
    && apt-get install -y procps 
COPY init_server.sh .
CMD sleep 30000
