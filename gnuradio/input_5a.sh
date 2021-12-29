#!/bin/bash
cat pipe_command.bin | nc -w 1 127.0.0.1 1234
for i in {1..100}; do cat randomdata.bin | nc -w 1 127.0.0.1 1234; done
