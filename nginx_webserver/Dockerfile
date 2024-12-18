FROM nginx
RUN apt-get update \
    && apt-get install -y netcat-openbsd\
    && apt-get install -y iproute2 \
    && apt-get install -y procps \
    && apt-get install -y vim 
COPY init_server.sh .
COPY ./html/ /opt/html/
CMD ./init_server.sh
