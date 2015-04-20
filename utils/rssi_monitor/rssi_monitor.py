#!/usr/bin/env python
#
# Copyright 2013 Bastian Bloessl <bloessl@ccs-labs.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import socket
import struct
import threading
import time
import signal
import gtk
import gobject

import rssi_window

USRP_IP = "192.168.10.4"
LOCAL_IP = "192.168.10.1"
UDP_PORT = 49160

gobject.threads_init()

class MainLoop(threading.Thread):

	def __init__(self):
		super(MainLoop, self).__init__()
		self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
		self.sock.bind((LOCAL_IP, UDP_PORT))
		self.sock.settimeout(2)
		self._stopped = False
		signal.signal(signal.SIGINT, exitHandler)
		print "initalized"

	def stop(self):
		self._stopped = True

	def stopped(self):
		return self._stopped

	def run(self):
		print "thread running"
		while not(self.stopped()):
			try:
				print "building rssi query"
				rssi_query = struct.pack("257B", *tuple([6] + [0] * 256))
				print "sending rssi query"
				self.sock.sendto(rssi_query, (USRP_IP, UDP_PORT))
				print "rssi query sent"
				data, addr = self.sock.recvfrom(1000)
				print "rssi response received"
				a = struct.unpack("!I", data[2:6])
				print a

				#if(a[0] < 100):
				#	rssiWindow.toggle_lamp()
				rssiWindow.update(a[0])

			except Exception as inst:
				print type(inst)     # the exception instance
				print inst.args      # arguments stored in .args
				print inst
				pass
		print "main loop finished"
		rssiWindow.close_from_mainthread()
		gtk.main_quit()


def exitHandler(signum, frame):
	print "in exit handler"
	rssiWindow._stopped = True


if __name__ == "__main__":
	signal.signal(signal.SIGINT, exitHandler)

	mainThread = MainLoop()
	rssiWindow = rssi_window.RssiWindow(mainThread)

	mainThread.start()

	gtk.main()
	print "ciao"

