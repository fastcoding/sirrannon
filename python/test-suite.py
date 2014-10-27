#!/usr/bin/env python3
import sys
from subprocess import Popen
from time import time, sleep
import subprocess
from optparse import OptionParser
from shlex import split

HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'

DEFAULT_TIMEOUT = 5.0

class test:
    
    def __init__( self ):
        self.iStep = 0
        self.sDir = '.'
        self.iErrors = 0
        self.lCmdObject = []
        self.oOptions = None
        self.oArgs = []
 
    def main( self, *args ):
         ## Flags
        oParse = OptionParser()
        oParse.set_defaults(svn=False, bootstrap=False, configure=False, make=False, build=False, test=False, unix=False, mingw32=False, mingw64=False)
        oParse.add_option("-s", "--svn", action="store_true", dest="svn")
        oParse.add_option("-b", "--bootstrap", action="store_true", dest="bootstrap")
        oParse.add_option("-c", "--configure", action="store_true", dest="configure")
        oParse.add_option("-m", "--make", action="store_true", dest="make")
        oParse.add_option("-U", "--unix", action="store_true", dest="unix")
        oParse.add_option("-W", "--mingw32", action="store_true", dest="mingw32")
        oParse.add_option("-Z", "--mingw64", action="store_true", dest="mingw64")
        self.oOptions, self.oArgs = oParse.parse_args()
                
        ## Do a number of tests
        if self.oOptions.unix:
            ## Obtaining
            self.sDir = '.'
            if self.oOptions.svn:
                self.callCommand( ["svn", "co", "https://svn.atlantis.ugent.be/svn/video/sirannon/trunk", "build-test"], bSeverity=True )
                                           
            self.sDir = "build-test"
            self.test()
             
        if self.oOptions.mingw32:
            self.sDir = "build-mingw32"
            self.test( bWindows=True )
            
        if self.oOptions.mingw64:
            self.sDir = "build-mingw64"
            self.test( bWindows=True )            
    
        ## How many errors?: 
        if self.iErrors:
            print( "TEST: FAILED: {} ERRORS".format(self.iErrors) )
            sys.exit( 1 )
        else:
            print( "TEST: SUCCES: 0 ERRORS" )
            sys.exit( 0 ) 
            
    def test(self, bWindows=False):
        ## Bootstrap and make
        self.callCommand( ["svn", "update"], bSeverity=False )     
        if self.oOptions.bootstrap:
            self.callCommand( ["python/bootstrap.py"], bSeverity=True )
        if self.oOptions.configure:
            self.callCommand( ["./configure", "--enable-local"], bSeverity=True )
        if self.oOptions.make:
            self.callCommand( ["make"], bSeverity=True )
            if bWindows:
                self.callCommand( ["cp", "src/sirannon.exe", "src/sirannon"], bSeverity=True )
             
        ## Perform a series of tests
        if self.oArgs:
            if "all" in self.oArgs:
                self.oArgs = "udp", "tcp", "http", "rtmp", "rtsp", "html5", "rtsp-cross", "http-cross", "rtmp-cross", "loss", "rtmp-cross"
       
            #if not bWindows:
            #    self.callCommand( ["./python/sirannon.py"])            
             
            ## All demos
            # UDP
            if "udp" in self.oArgs:                      
                self.callCommand( split("vlc udp://@:1234"),
                                  split("./src/sirannon ../dat/xml/demo4B.xml 127.0.0.1"),
                                  split("./src/sirannon ../dat/xml/demo4A.xml ../dat/media/demo/demo.mov 127.0.0.1 test.ts"),
                                  fTimeOut=60. )
            # RTP
            if "rtp" in self.oArgs:
                self.callCommand( ["vlc", "rtp://@:1234"],
                                  ["./src/sirannon", "../dat/xml/demo1.xml", "../dat/media/demo/demo.mov", "127.0.0.1"],
                                  fTimeOut=60. )
                
                self.callCommand( ["vlc", "rtp://@:1234"],
                                  ["./src/sirannon", "../dat/xml/demo1.xml", "../dat/media/demo/demo.264", "127.0.0.1"],
                                  fTimeOut=60. )
            # RTSP
            if "rtsp" in self.oArgs:    
                self.callCommand( ["./src/sirannon", "../dat/xml/demo2.xml", "../dat/media", "5554"],
                                  ["vlc", "rtsp://localhost:5554/FILE/demo/demo.mov"],
                                  fTimeOut=60. )
                
                self.callCommand( ["./src/sirannon", "../dat/xml/demo2.xml", "../dat/media", "5554"],
                                  ["vlc", "rtsp://localhost:5554/FILE/demo/demo.avi"],
                                  fTimeOut=60. )
                
                self.callCommand( ["./src/sirannon", "../dat/xml/demo2.xml", "../dat/media", "5554"],
                                  ["vlc", "rtsp://localhost:5554/FILE/demo/demo.mpeg"],
                                  fTimeOut=60. )                
           
            if "rtmp" in self.oArgs:
                # RTMP
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                                  ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp.html"],
                                  fTimeOut=60. )
                
                # RTMPT
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                                  ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmpt.html"],
                                  fTimeOut=60. )
                
                # RTMP, WEBM
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                                  ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp-webm.html"],
                                  fTimeOut=60. )
                             
            if "http" in self.oArgs:     
                # HTTP, FLV(H.264/MP4A) on FlashPlayer
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["chromium-browser", "../dat/flowplayer-3.1.5/example/http.html"],
                          fTimeOut=60. )
                
                # HTTP, VP6 on FlashPlayer
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/HTTP@FLV/mp4/clock.mp4"],
                          fTimeOut=60. )                    
            
            if "html5" in self.oArgs:
                # HTML 5 - WEBM
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                                  ["chromium-browser", "../dat/html5/webm.html"],
                                  fTimeOut=60. )
                       
                # HTML 5 - MP4 to WEBM
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                                  ["chromium-browser", "../dat/html5/webm-t.html"],
                                  fTimeOut=60. )
              
            if "http-cross" in self.oArgs:
               # RTSP -> HTTP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/RTSP-proxy@FLV/localhost:5554/FILE/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # RTMP -> HTTP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/RTMP-proxy@FLV/localhost:1935/FILE/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # HTTP -> HTTP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/HTTP-proxy@FLV/localhost:8080/FILE@FLV/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # APPLE -> HTTP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/HTTP-proxy@FLV/localhost:8080/M3U/localhost:8080/FILE@TS/mp4/clock.mp4"],
                          fTimeOut=60. )
               
            if "apple-cross" in self.oArgs:
               # RTSP -> APPLE
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/M3U/localhost:8080/RTSP-proxy@TS/localhost:5554/FILE/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # RTMP -> APPLE
               self.callCommand( ["./src/sirannon", "-vvv", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/M3U/localhost:8080/RTMP-proxy@TS/localhost:1935/FILE/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # HTTP -> APPLE
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/M3U/localhost:8080/HTTP-proxy@TS/localhost:8080/FILE@FLV/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # APPLE -> APPLE
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/M3U/localhost:8080/HTTP-proxy@TS/localhost:8080/M3U/localhost:8080/FILE@TS/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # HTTP(WEBM) -> APPLE
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "http://localhost:8080/M3U/localhost:8080/HTTP-proxy@TS/localhost:8080/HTTP/webm/da.webm"],
                          fTimeOut=60. )
               
            if "rtsp-cross" in self.oArgs:
               # HTTP -> RTSP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "rtsp://localhost:5554/HTTP-proxy/localhost:8080/FILE@FLV/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # RTMP -> RTSP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "rtsp://localhost:5554/RTMP-proxy/localhost:1935/FILE@FLV/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # RTSP -> RTSP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                          ["vlc", "rtsp://localhost:5554/RTSP-proxy/localhost:5554/FILE@FLV/mp4/clock.mp4"],
                          fTimeOut=60. )
               
               # APPLE -> RTSP
               self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                      ["vlc", "rtsp://localhost:5554/HTTP-proxy/localhost:8080/M3U/localhost:8080/FILE@TS/mp4/clock.mp4"],
                      fTimeOut=60. )
               
            if "rtmp-cross" in self.oArgs:
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                         ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp-rtmp.html"],
                          fTimeOut=60. )
                
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                         ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp-rtsp.html"],
                          fTimeOut=60. )
                 
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                         ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp-http.html"],
                          fTimeOut=60. )
                
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                         ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp-rtmpt.html"],
                          fTimeOut=60. )
                
                self.callCommand( ["./src/sirannon", "../dat/xml/media-server.xml", "../dat/media"],
                         ["chromium-browser", "../dat/flowplayer-3.1.5/example/rtmp-apple.html"],
                          fTimeOut=60. )
         
            if "loss" in self.oArgs:  
                # TCP
                self.callCommand( split("./src/sirannon ../dat/xml/demo5A.xml ../dat/xml/demo5B.xml ../dat/xml/demo5B.xml ../dat/media/demo/demo.mov ../dat/media/demo/demo.mpeg ../dat/media/demo/demo.avi 1 8000 80"),
                                  fTimeOut=60. )                
                              
                # Loss
                self.callCommand( split("./src/sirannon ../dat/xml/demo3.xml ../dat/media/demo/demo.mpeg demo.m1v 0.001 0.005"),
                                  fTimeOut=60. )

    def callCommand( self, *lProgs, bShell=False, bSeverity=False, fTimeOut=0., bTimeOutIsError=True ):
        ## Nth step
        self.iStep += 1
        
        ## Invoke
        self.lCmdObject = []
        for i,lProg in enumerate(lProgs):
            oCmd = Popen( lProg, cwd=self.sDir, shell=bShell )
            self.lCmdObject.append( oCmd )
            print( HEADER + "{3}-{0:03}: RUNNING CMD({1}) PID({2})".format(self.iStep, lProg, oCmd.pid, self.sDir) + ENDC )
            sleep( 2. )
            
        ## Wait
        bTimeOut = self.wait( fTimeOut )
            
        ## Stop all commands
        for oCmd in self.lCmdObject:
            if oCmd.returncode is None:
               print( "{2}-{0:03}: KILL PID({1})".format(self.iStep, oCmd.pid, self.sDir) )   
               oCmd.terminate()
               oCmd.wait()
      
        ## Error?
        bError = False
        
        if bTimeOut and bTimeOutIsError:
            bError = True
        else:
            for oCmd in self.lCmdObject:
                if oCmd.returncode == -15:
                    pass
                elif oCmd.returncode != 0:
                    bError = True
                    break
        if bError:
            self.iErrors += 1
             
        ## Evaluate the return code
        sCode = str( [ x.returncode for x in self.lCmdObject ] ) 
            
        if bError:
            if bSeverity:
                if bTimeOut:
                    print( "{2}-{0:03}: CRITICAL TIME-OUT: {1}".format(self.iStep,sCode, self.sDir) )
                else:
                    print( "{2}-{0:03}: CRITICAL FAILURE: {1}".format(self.iStep,sCode, self.sDir) )
                sys.exit(1)
                
            elif bTimeOut:
                print( "{2}-{0:03}: TIME-OUT: {1}".format(self.iStep,sCode, self.sDir) )
            else:
                print( "{2}-{0:03}: FAILURE: {1}".format(self.iStep,sCode, self.sDir) )
        else:
            print( "{2}-{0:03}: SUCCES: {1}".format(self.iStep,sCode, self.sDir) )
            
    def wait( self, fTimeOut ):
        fEnd = time() + fTimeOut
        bWait = False
        
        while( True ):
            lStatus = [ oCmd.poll() for oCmd in self.lCmdObject  ]
            if [iStatus for iStatus in lStatus if iStatus is not None and True]:
                return False
            elif [iStatus for iStatus in lStatus if iStatus is None]:
                if fTimeOut > 0. and time() > fEnd:
                    return True
                else:
                    sleep(.1)
            else:
                break
        return False

if __name__ == '__main__':
    test().main(*sys.argv)
