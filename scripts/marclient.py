#!/usr/bin/env python

import threading
import xmlrpclib
import socket
import time
import threading
import json

MCAST_ADDR = "224.0.0.1"
MCAST_PORT = 19678
STOP_MSG = "STOP"
INTF = "172.31.0.2"

discover_time = None

class ServiceDiscover(threading.Thread):

    def __init__(self, sock):
        threading.Thread.__init__(self)
        self.running = threading.Event()
        self.sock = sock
        self.servers = {}

    def run(self):
        self.running.set()
        while self.running.is_set():
            data, addr = self.sock.recvfrom(1024)

            if data == STOP_MSG: break

            latency = time.time() - discover_time
            stats = json.loads(data)
            stats['latency'] = latency
            stats['ip'] = addr[0]
            self.update_service(stats)

    def update_service(self, stats):
        print "MAR Service found => %s" % stats
        self.servers[stats['ip']] = stats
 
    def stop(self):
        self.running.clear()
        self.sock.sendto(STOP_MSG, self.sock.getsockname())


def init_usock(intf=INTF, addr='', port=0):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sock.setsockopt(socket.SOL_IP, socket.IP_MULTICAST_IF, \
        socket.inet_aton(intf) + socket.inet_aton('127.0.0.1'))

    sock.bind((addr, port))
    return sock

def mar_request(mar_server, path):
    fp = open(path, 'rb')
    data = fp.read()
    fp.close()

    url = "http://%s:%s/" % (mar_server['ip'], mar_server['port'])
    print "\nSending MAR task to %s" % url
    proxy = xmlrpclib.ServerProxy(url)
    #print proxy.system.listMethods()
    contents = proxy.do_mar_task(xmlrpclib.Binary(data))
    print "Received MAR response from %s" % url

    fp2 = open(mar_server['ip'] + "_"+path, "wb")
    fp2.write(contents.data)
    fp2.close()

def main():

    sock = init_usock()

    sdthread = ServiceDiscover(sock)
    sdthread.start()

    addr = (MCAST_ADDR, MCAST_PORT)
    global discover_time

    try:
        while True:
            sdthread.servers.clear()
            path = raw_input('\nEnter image file name: ')
            discover_time = time.time()
            print "\nSending discover request over multicast"
            sock.sendto('discover', addr)

            while len(sdthread.servers) == 0:
                time.sleep(3)

            mar_server = None
            for key, value in sdthread.servers.items():
                if mar_server == None:
                    mar_server = value
                else:
                    if value['latency'] < mar_server['latency']:
                        mar_server = value

            print "\nServer %s at %s has lowest latency" % \
                (mar_server['name'], mar_server['ip'])
            mar_request(mar_server, path)
    except KeyboardInterrupt:
        sdthread.stop()


if __name__ == "__main__":
    main()
