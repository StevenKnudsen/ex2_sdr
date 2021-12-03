#!/bin/bash
nc -l 127.0.0.1 4321 | tee output_data.bin
