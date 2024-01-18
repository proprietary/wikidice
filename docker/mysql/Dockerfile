FROM mariadb:11.2-jammy

RUN apt-get update && apt-get install -y curl

ENV MARIADB_ALLOW_EMPTY_ROOT_PASSWORD=1

COPY WIKIDUMP_URL.txt /tmp/WIKIDUMP_URL.txt

RUN mariadb -e "CREATE DATABASE wikidb;"

RUN curl -L "$(cat /tmp/WIKIDUMP_URL)" | gunzip | mariadb