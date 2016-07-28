#!/bin/bash

./hello &

echo " ======  test use socat at tcp:9995  ================"
socat STDIO tcp:127.0.0.1:9995
socat STDIO tcp:127.0.0.1:9995
socat STDIO tcp:127.0.0.1:9995


echo " ======  test use socat on unix domain at .haha  ============="
socat STDIO unix:.haha
socat STDIO unix:.haha
socat STDIO unix:.haha


echo " ======  test use our 'client' at tcp:9995  ============="
./hehe
./hehe
./hehe

echo " ==== send ctrl-C to 'hello'  ===="
pkill -2 hello


