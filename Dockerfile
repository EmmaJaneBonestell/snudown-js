FROM emscripten/emsdk:3.1.18

RUN apt-get update
RUN apt-get install gperf
