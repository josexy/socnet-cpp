FROM debian:stable-slim

RUN apt-get update -y && apt-get install -y \
    git \
    build-essential \
    make \
    cmake \
    g++ \
    zlib1g-dev\ 
    libssl-dev

ENV APP_ROOT=/webapp

WORKDIR $APP_ROOT

RUN git clone https://github.com/josexy/socnet-cpp && \
    cd socnet-cpp/build &&  \
    cmake .. && \
    make 

ENV SOCNET_ROOT="$APP_ROOT/socnet-cpp/build"

WORKDIR $SOCNET_ROOT

EXPOSE 5555

CMD ["./socnet"]