#!/usr/bin/env python

import threading
import socket
import logging
import StringIO
import xmlrpclib
import platform
import json
from PIL import Image, ImageOps, ImageDraw, ImageFont
from SimpleXMLRPCServer import SimpleXMLRPCServer

MCAST_ADDR = "224.0.0.1"
MCAST_PORT = 19678
STOP_MSG = "STOP"
INTF = "172.31.0.2"

class ServiceAnnouncer(threading.Thread):

    def __init__(self, sock, port, stats):
        threading.Thread.__init__(self)
        self.port = port
        self.running = threading.Event()
        self.sock = sock
        self.stats = stats

    def run(self):
        self.running.set()
        msg = json.dumps(self.stats)
        while self.running.is_set():
            data, addr = self.sock.recvfrom(1024)

            if data == STOP_MSG: break

            print "ServiceAnnounceThread : rx : %s" % data
            self.sock.sendto(msg, addr)
            print "ServiceAnnounceThread : tx : %s" % msg

    def stop(self):
        self.running.clear()
        self.sock.sendto('', self.sock.getsockname())


class MARService:

    def test(self):
        return 'hello world'

    def do_mar_task(self, data):
        print 'MARService : starting mar task'

        fp = StringIO.StringIO(data.data)
        im = Image.open(fp)

        # do some image processing
        im2 = ImageOps.invert(im).rotate(180)
        draw = ImageDraw.Draw(im2)
        font = ImageFont.truetype("/usr/share/fonts/truetype/freefont/FreeSans.ttf", 30)
        wmark = "image processed by %s" % platform.node()
        draw.text((5,5), wmark, font=font)
        del draw

        fp2 = StringIO.StringIO()
        im2.save(fp2, 'JPEG')
        contents = fp2.getvalue()

        fp.close()
        fp2.close()
        print 'MARService : ending mar task'
        return xmlrpclib.Binary(contents)


def init_msock(intf=INTF, port=MCAST_PORT, addr=MCAST_ADDR):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        #s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
    except AttributeError as ex:
        logging.exception(ex)

    s.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_TTL, 255)
    s.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_LOOP, 1)

    s.bind(('', port))

    print intf

    s.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_IF, \
        socket.inet_aton(intf) + socket.inet_aton('127.0.0.1'))

    s.setsockopt(socket.SOL_IP, socket.IP_ADD_MEMBERSHIP, \
        socket.inet_aton(addr) + socket.inet_aton(intf))

    return s

def get_service_stats(port):
    stats = {}
    stats['port'] = port
    stats['machine'] = platform.machine()
    stats['name'] = platform.node()
    return stats

def main():

    port = 8123
    msock = init_msock()
    stats = get_service_stats(port)

    announce_thread = ServiceAnnouncer(msock, port, stats)
    announce_thread.start()

    server = SimpleXMLRPCServer(("", port))
    server.register_introspection_functions()
    server.register_instance(MARService())

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        announce_thread.stop()

if __name__ == "__main__":
    main()
