#!/bin/bash
cat test_input.bin | nc -u -w1 127.0.0.1 1234
