#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# SPDX-License-Identifier: GPL-3.0
#
# GNU Radio Python Flow Graph
# Title: Not titled yet
# GNU Radio version: 3.8.2.0

from distutils.version import StrictVersion

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print("Warning: failed to XInitThreads()")

from gnuradio import blocks
from gnuradio import digital
from gnuradio import gr
from gnuradio.filter import firdes
import sys
import signal
from PyQt5 import Qt
from argparse import ArgumentParser
from gnuradio.eng_arg import eng_float, intx
from gnuradio import eng_notation
import satellites.hier

from gnuradio import qtgui

class endurosat_e2e(gr.top_block, Qt.QWidget):

    def __init__(self):
        gr.top_block.__init__(self, "Not titled yet")
        Qt.QWidget.__init__(self)
        self.setWindowTitle("Not titled yet")
        qtgui.util.check_set_qss()
        try:
            self.setWindowIcon(Qt.QIcon.fromTheme('gnuradio-grc'))
        except:
            pass
        self.top_scroll_layout = Qt.QVBoxLayout()
        self.setLayout(self.top_scroll_layout)
        self.top_scroll = Qt.QScrollArea()
        self.top_scroll.setFrameStyle(Qt.QFrame.NoFrame)
        self.top_scroll_layout.addWidget(self.top_scroll)
        self.top_scroll.setWidgetResizable(True)
        self.top_widget = Qt.QWidget()
        self.top_scroll.setWidget(self.top_widget)
        self.top_layout = Qt.QVBoxLayout(self.top_widget)
        self.top_grid_layout = Qt.QGridLayout()
        self.top_layout.addLayout(self.top_grid_layout)

        self.settings = Qt.QSettings("GNU Radio", "endurosat_e2e")

        try:
            if StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
                self.restoreGeometry(self.settings.value("geometry").toByteArray())
            else:
                self.restoreGeometry(self.settings.value("geometry"))
        except:
            pass

        ##################################################
        # Variables
        ##################################################
        self.fsk_dev = fsk_dev = 2400
        self.baud_bit = baud_bit = 9600
        self.tx_gain = tx_gain = 0.8
        self.spsym = spsym = 160
        self.sensitivity = sensitivity = 2*3.14159*(fsk_dev/baud_bit)
        self.rx_gain = rx_gain = 20
        self.center_freq = center_freq = 435000000
        self.baud_byte = baud_byte = baud_bit/8

        ##################################################
        # Blocks
        ##################################################
        self.satellites_sync_to_pdu_packed_0_0 = satellites.hier.sync_to_pdu_packed(
            packlen=3,
            sync='01111110',
            threshold=0,
        )
        self.digital_gfsk_mod_0_0 = digital.gfsk_mod(
            samples_per_symbol=spsym,
            sensitivity=sensitivity,
            bt=0.5,
            verbose=False,
            log=False)
        self.digital_gfsk_demod_0 = digital.gfsk_demod(
            samples_per_symbol=spsym,
            sensitivity=sensitivity,
            gain_mu=0.175,
            mu=0.5,
            omega_relative_limit=0.005,
            freq_error=0.0,
            verbose=False,
            log=False)
        self.blocks_vector_source_x_0_0 = blocks.vector_source_b((170, 170, 170, 170, 170, 170, 170, 170, 170, 170, 126, 2, 69, 83, 58, 83), True, 1, [])
        self.blocks_throttle_0 = blocks.throttle(gr.sizeof_char*1, baud_bit,True)
        self.blocks_socket_pdu_0 = blocks.socket_pdu('TCP_CLIENT', '127.0.0.1', '4321', 10000, False)



        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.satellites_sync_to_pdu_packed_0_0, 'out'), (self.blocks_socket_pdu_0, 'pdus'))
        self.connect((self.blocks_throttle_0, 0), (self.digital_gfsk_mod_0_0, 0))
        self.connect((self.blocks_vector_source_x_0_0, 0), (self.blocks_throttle_0, 0))
        self.connect((self.digital_gfsk_demod_0, 0), (self.satellites_sync_to_pdu_packed_0_0, 0))
        self.connect((self.digital_gfsk_mod_0_0, 0), (self.digital_gfsk_demod_0, 0))


    def closeEvent(self, event):
        self.settings = Qt.QSettings("GNU Radio", "endurosat_e2e")
        self.settings.setValue("geometry", self.saveGeometry())
        event.accept()

    def get_fsk_dev(self):
        return self.fsk_dev

    def set_fsk_dev(self, fsk_dev):
        self.fsk_dev = fsk_dev
        self.set_sensitivity(2*3.14159*(self.fsk_dev/self.baud_bit))

    def get_baud_bit(self):
        return self.baud_bit

    def set_baud_bit(self, baud_bit):
        self.baud_bit = baud_bit
        self.set_baud_byte(self.baud_bit/8)
        self.set_sensitivity(2*3.14159*(self.fsk_dev/self.baud_bit))
        self.blocks_throttle_0.set_sample_rate(self.baud_bit)

    def get_tx_gain(self):
        return self.tx_gain

    def set_tx_gain(self, tx_gain):
        self.tx_gain = tx_gain

    def get_spsym(self):
        return self.spsym

    def set_spsym(self, spsym):
        self.spsym = spsym

    def get_sensitivity(self):
        return self.sensitivity

    def set_sensitivity(self, sensitivity):
        self.sensitivity = sensitivity

    def get_rx_gain(self):
        return self.rx_gain

    def set_rx_gain(self, rx_gain):
        self.rx_gain = rx_gain

    def get_center_freq(self):
        return self.center_freq

    def set_center_freq(self, center_freq):
        self.center_freq = center_freq

    def get_baud_byte(self):
        return self.baud_byte

    def set_baud_byte(self, baud_byte):
        self.baud_byte = baud_byte





def main(top_block_cls=endurosat_e2e, options=None):

    if StrictVersion("4.5.0") <= StrictVersion(Qt.qVersion()) < StrictVersion("5.0.0"):
        style = gr.prefs().get_string('qtgui', 'style', 'raster')
        Qt.QApplication.setGraphicsSystem(style)
    qapp = Qt.QApplication(sys.argv)

    tb = top_block_cls()

    tb.start()

    tb.show()

    def sig_handler(sig=None, frame=None):
        Qt.QApplication.quit()

    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, sig_handler)

    timer = Qt.QTimer()
    timer.start(500)
    timer.timeout.connect(lambda: None)

    def quitting():
        tb.stop()
        tb.wait()

    qapp.aboutToQuit.connect(quitting)
    qapp.exec_()

if __name__ == '__main__':
    main()
