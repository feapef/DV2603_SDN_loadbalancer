FROM nginx

COPY init_server.sh /usr/local/bin/init_server.sh
COPY ./html/ /opt/html/

RUN apt-get update \
    && apt-get install -y netcat-openbsd\
    && apt-get install -y iproute2 \
    && apt-get install -y dsniff \
    && apt-get install -y procps \
    && apt-get install -y vim 
RUN chmod +x /usr/local/bin/init_server.sh

ENTRYPOINT ["/usr/local/bin/init_server.sh"]
