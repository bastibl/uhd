#!/usr/bin/env python


import socket
import struct
import time

USRP_IP = "192.168.10.4"
LOCAL_IP = "192.168.10.1"
UDP_PORT = 49160


GET_SETTINGS    = 0x00
GET_BACKOFF     = 0x01
GET_STATUS      = 0x02
GET_READBACK    = 0x03
GET_DUMMY       = 0x04
GET_THRESHOLD   = 0x05
GET_RSSI        = 0x06

SET_SETTINGS    = 0x10
SET_BACKOFF     = 0x11
SET_DUMMY       = 0x12
SET_READBACK    = 0x13

TEXT_MESSAGE    = 0xFF


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((LOCAL_IP, UDP_PORT))
sock.settimeout(2)


### Average RSSI
query = struct.pack("257B", *tuple([GET_RSSI] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!I", data[2:6])
print "average rssi: %.6d" % a[0]

### Backoff
query = struct.pack("257B", *tuple([GET_BACKOFF] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!IIIIIII", data[2:30])
print "Backoffs:"
print "  round 0: " + str(a[0])
print "  round 1: " + str(a[1])
print "  round 2: " + str(a[2])
print "  round 3: " + str(a[3])
print "  round 4: " + str(a[4])
print "  round 5: " + str(a[5])

### Status
query = struct.pack("257B", *tuple([GET_STATUS] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!BBBBBBBBH", data[2:12])
states = ["IDLE", "DIFS", "SENDING", "SLOT", "SEND", "DIFS_GO"]
print "status:"
print "  run:      " + str(a[0])
print "  error:    " + str(a[1])
print "  state:    " + str(a[2]) + " (" + states[a[2]] + ")"
print "  ena:      " + str(a[3])
print "  free:     " + str(a[4])
print "  round:    " + str(a[5])
print "  ready:    " + str(a[6])
print "  reserved: " + str(a[7])
print "  reserved: " + str(a[8])

### Settings
query = struct.pack("257B", *tuple([GET_SETTINGS] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!BBHI", data[2:10])
print "settings:"
print "  samples:        " + str(a[0])
print "  read_back_addr: " + str(a[1])
print "  threshold:      " + str(a[2])


### Threshold (over readback)
query = struct.pack("257B", *tuple([SET_READBACK, 1, 8] + [0] * 254))
sock.sendto(query, (USRP_IP, UDP_PORT))
time.sleep(0.01)
query = struct.pack("257B", *tuple([GET_READBACK] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!I", data[2:6])
print "Threshold: " + str(a[0])

### Slottime (over readback)
query = struct.pack("257B", *tuple([SET_READBACK, 1, 7] + [0] * 254))
sock.sendto(query, (USRP_IP, UDP_PORT))
time.sleep(0.01)
query = struct.pack("257B", *tuple([GET_READBACK] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!I", data[2:6])
print "Slottime: " + str(a[0])

### DIFS (over readback)
query = struct.pack("257B", *tuple([SET_READBACK, 1, 9] + [0] * 254))
sock.sendto(query, (USRP_IP, UDP_PORT))
time.sleep(0.01)
query = struct.pack("257B", *tuple([GET_READBACK] + [0] * 256))
sock.sendto(query, (USRP_IP, UDP_PORT))
data, addr = sock.recvfrom(1000)
a = struct.unpack("!I", data[2:6])
print "DIFS: " + str(a[0])
