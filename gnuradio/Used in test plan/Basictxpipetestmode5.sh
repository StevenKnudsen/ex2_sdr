#!/bin/bash
cat pipe_command_mode5.bin | nc -w 1 127.0.0.1 1234
cat "ES".bin | nc -w 1 127.0.0.1 1234
