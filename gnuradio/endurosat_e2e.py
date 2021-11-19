#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# GNU Radio version: 3.8.2.0

from gnuradio import blocks
from gnuradio import digital
from gnuradio import gr
from gnuradio.filter import firdes
import sys
import signal
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import satellites.components.datasinks
import satellites.components.demodulators
import satellites.hier


class endurosat_e2e(gr.top_block):

    def __init__(self):
        gr.top_block.__init__(self, "Not titled yet")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 192000
        self.fsk_dev = fsk_dev = 600
        self.baud = baud = 2400
        self.aug = aug = 1
        self.sensitivity = sensitivity = 2*3.14159*(fsk_dev/samp_rate)*aug
        self.points = points = int(16*8*(samp_rate/baud))
        self.decim = decim = 100

        ##################################################
        # Blocks
        ##################################################
        self.satellites_sync_to_pdu_packed_0_0 = satellites.hier.sync_to_pdu_packed(
            packlen=5,
            sync='01111110',
            threshold=0,
        )
        self.satellites_hexdump_sink_0 = satellites.components.datasinks.hexdump_sink(options="")
        self.satellites_fsk_demodulator_0 = satellites.components.demodulators.fsk_demodulator(baudrate = baud, samp_rate = samp_rate, iq = True, subaudio = False, options="")
        self.digital_gfsk_mod_0 = digital.gfsk_mod(
            samples_per_symbol=int(samp_rate/baud),
            sensitivity=sensitivity,
            bt=0.5,
            verbose=False,
            log=False)
        self.digital_binary_slicer_fb_0 = digital.binary_slicer_fb()
        self.blocks_udp_source_0 = blocks.udp_source(gr.sizeof_char*1, '127.0.0.1', 1234, 1472, True)
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_char*1, baud,True)
        self.blocks_socket_pdu_0 = blocks.socket_pdu('TCP_CLIENT', '127.0.0.1', '4321', 10000, False)



        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.satellites_sync_to_pdu_packed_0_0, 'out'), (self.blocks_socket_pdu_0, 'pdus'))
        self.msg_connect((self.satellites_sync_to_pdu_packed_0_0, 'out'), (self.satellites_hexdump_sink_0, 'in'))
        self.connect((self.blocks_throttle_0, 0), (self.digital_gfsk_mod_0, 0))
        self.connect((self.blocks_udp_source_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.digital_binary_slicer_fb_0, 0), (self.satellites_sync_to_pdu_packed_0_0, 0))
        self.connect((self.digital_gfsk_mod_0, 0), (self.satellites_fsk_demodulator_0, 0))
        self.connect((self.satellites_fsk_demodulator_0, 0), (self.digital_binary_slicer_fb_0, 0))


    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.set_points(int(16*8*(self.samp_rate/self.baud)))
        self.set_sensitivity(2*3.14159*(self.fsk_dev/self.samp_rate)*self.aug)

    def get_fsk_dev(self):
        return self.fsk_dev

    def set_fsk_dev(self, fsk_dev):
        self.fsk_dev = fsk_dev
        self.set_sensitivity(2*3.14159*(self.fsk_dev/self.samp_rate)*self.aug)

    def get_baud(self):
        return self.baud

    def set_baud(self, baud):
        self.baud = baud
        self.set_points(int(16*8*(self.samp_rate/self.baud)))
        self.blocks_throttle_0.set_sample_rate(self.baud)

    def get_aug(self):
        return self.aug

    def set_aug(self, aug):
        self.aug = aug
        self.set_sensitivity(2*3.14159*(self.fsk_dev/self.samp_rate)*self.aug)

    def get_sensitivity(self):
        return self.sensitivity

    def set_sensitivity(self, sensitivity):
        self.sensitivity = sensitivity

    def get_points(self):
        return self.points

    def set_points(self, points):
        self.points = points

    def get_decim(self):
        return self.decim

    def set_decim(self, decim):
        self.decim = decim





def main(top_block_cls=endurosat_e2e, options=None):
    tb = top_block_cls()

    def sig_handler(sig=None, frame=None):
        tb.stop()
        tb.wait()

        sys.exit(0)

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    tb.start()

    try:
        input('Press Enter to quit: ')
    except EOFError:
        pass
    tb.stop()
    tb.wait()


if __name__ == '__main__':
    main()
