 FROM gcc:4.9

COPY . /usr/src/app

WORKDIR /usr/src/app

RUN ./configure --with-pydebug && make clean && make -s -j2


#FROM ubuntu:16.04

#WORKDIR /app

#COPY . .

#RUN apt-get update && apt-get install -y build-essential

#RUN ./configure --with-pydebug && make clean && make -s -j2
