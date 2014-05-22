#Copyright Jon Berg , turtlemeat.com

import string,cgi,time,urlparse,subprocess
from os import curdir, sep
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer

class MyHandler(BaseHTTPRequestHandler):

    oclSobelFilter = None

    def do_GET(self):
        try:
            self.send_response(200)
            self.send_header('Content-type', 'text/plain')
            self.end_headers()
            self.wfile.write("hello world")
        except:
            self.send_error(404,'failure at: %s' % self.path)
     
    def do_POST(self):
        # this creates RPC dictionary
        presults = urlparse.urlparse(self.path)
        results = urlparse.parse_qs(presults[4])

        if results['m'][0] == "sobelfilter":
            try:
                length = int(self.headers["content-length"])
                data = self.rfile.read(length)

                self.send_response(200)
                self.send_header("Content-type", "image/jpeg")
                self.end_headers()

                f = open("tmp.jpg", "wb")
                f.write(data)
                f.close()

                f_in = open("tmp.jpg", "rb")
                f_out = open("tmp.ppm", "wb")
                subprocess.check_call(["jpegtopnm"], stdin=f_in, stdout=f_out)
                f_in.close()
                f_out.close()

                self.oclSobelFilter.communicate("go\n")

                f_in = open("out.ppm", "rb")
                f_out = open("out.jpg", "wb")
                subprocess.check_call(["pnmtojpeg"], stdin=f_in, stdout=f_out)
                f_in.close()
                f_out.close()

                f = open("out.jpg", "rb")
                data = f.read()
                f.close()

                self.wfile.write(data)
            except Exception as ex:
                print str(ex)

        self.send_error(404, "could not process request %s" % self.path)

def main():

    MyHandler.oclSobelFilter = subprocess.Popen(["./oclSobelFilter"], stdin=subprocess.PIPE)

    try:
        server = HTTPServer(('', 8080), MyHandler)
        print 'started httpserver...'
        server.serve_forever()
    except KeyboardInterrupt:
        print '^C received, shutting down server'
        server.socket.close()

if __name__ == '__main__':
    main()

