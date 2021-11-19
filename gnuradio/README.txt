This folder contains gnuradio work that is unrelated to the C++ meson project in the other folders here.

The example endurosat_test.grc requires GNURadio 3.8 and the Out of Tree module gr-satellites from Daniel Estevez to function.

The setup works with the USRP sample data that was taken at 5e6 samples/second at 2400 baud.

Endurosat_e2e is a test that uses network interfaces for input and output of data.

First run output_listen.sh, then run endurosat_e2e.py (or the GRC flow) and then run input.sh to trigger a packet.
