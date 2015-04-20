#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: Noise
# Generated: Thu Jul  3 14:05:04 2014
##################################################

from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.wxgui import forms
from grc_gnuradio import blks2 as grc_blks2
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import time
import wx

class noise(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="Noise")

        ##################################################
        # Variables
        ##################################################
        self.samp_rate = samp_rate = 2000000
        self.onoff = onoff = 0

        ##################################################
        # Blocks
        ##################################################
        self._onoff_chooser = forms.radio_buttons(
        	parent=self.GetWin(),
        	value=self.onoff,
        	callback=self.set_onoff,
        	label='onoff',
        	choices=[0, 1],
        	labels=['on', 'off'],
        	style=wx.RA_HORIZONTAL,
        )
        self.Add(self._onoff_chooser)
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
        	",".join(("addr=192.168.10.4", "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        self.uhd_usrp_sink_0.set_center_freq(5890000000, 0)
        self.uhd_usrp_sink_0.set_gain(30, 0)
        self.uhd_usrp_sink_0.set_csma_enable(False, 0)
        self.uhd_usrp_sink_0.set_csma_slottime(0, 0)
        self.uhd_usrp_sink_0.set_csma_threshold(0, 0)
        self.blocks_null_source_0 = blocks.null_source(gr.sizeof_gr_complex*1)
        self.blks2_selector_0 = grc_blks2.selector(
        	item_size=gr.sizeof_gr_complex*1,
        	num_inputs=2,
        	num_outputs=1,
        	input_index=onoff,
        	output_index=0,
        )
        self.analog_fastnoise_source_x_0 = analog.fastnoise_source_c(analog.GR_GAUSSIAN, 5, 0, 8192)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.analog_fastnoise_source_x_0, 0), (self.blks2_selector_0, 0))
        self.connect((self.blocks_null_source_0, 0), (self.blks2_selector_0, 1))
        self.connect((self.blks2_selector_0, 0), (self.uhd_usrp_sink_0, 0))



    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)

    def get_onoff(self):
        return self.onoff

    def set_onoff(self, onoff):
        self.onoff = onoff
        self._onoff_chooser.set_value(self.onoff)
        self.blks2_selector_0.set_input_index(int(self.onoff))

if __name__ == '__main__':
    import ctypes
    import sys
    if sys.platform.startswith('linux'):
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = noise()
    tb.Start(True)
    tb.Wait()
