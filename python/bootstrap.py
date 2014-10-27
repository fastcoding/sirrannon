#!/usr/bin/env python
import sys
from subprocess import call
from optparse import OptionParser
from shlex import split

HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'

class main:
    def __init__(self):
        self.ffmpeg = 'ffmpeg-0.9'
        self.jrtp = 'jrtplib-3.9.1'
        self.crcanvas = 'crcanvas-0.19'
    
    def error(self, str, exit=True):
        print(FAIL + str + ENDC)
        if exit:
            sys.exit(1)
            
    def msg(self, str):
        print(HEADER + str + ENDC)       
   
    def main(self, *args):
        # Command line
        parser = OptionParser()
        parser.set_defaults(gui=True, ffmpeg=True, jrtplib=True)
        parser.add_option("-F", "--disable-ffmpeg", action="store_false", dest="ffmpeg")
        parser.add_option("-J", "--disable-jrtplib", action="store_false", dest="jrtplib")
        parser.add_option("-G", "--disable-GUI", action="store_false", dest="gui")
        options, args = parser.parse_args()
        
        ## FFMpeg
        if options.ffmpeg:
            cmd = 'chmod 777 configure version.sh && ./configure --enable-debug=3 --disable-vaapi --disable-ffmpeg --disable-ffprobe --disable-ffserver --disable-ffplay --enable-memalign-hack --enable-version3 --enable-nonfree --enable-gpl --enable-postproc --enable-pthreads --enable-libvorbis --enable-libfaac --enable-libmp3lame --enable-libopencore-amrnb --enable-libopencore-amrwb --enable-libtheora --enable-libvpx --enable-libx264 --extra-cflags="-fexceptions" && make'
            self.msg("*** BUILDING FFMPEG ***\n" + cmd)
            returncode = call(cmd, shell=True, cwd="libs/"+self.ffmpeg)
            if returncode != 0:
                print(WARNING + "Could not configure ffmpeg, trying with less libraries" + ENDC)
                cmd = 'chmod +x configure version.sh && ./configure --disable-vaapi --disable-ffmpeg --disable-ffserver --disable-ffplay --disable-ffprobe --enable-memalign-hack --enable-version3 --enable-nonfree --enable-gpl --enable-postproc --enable-pthreads --enable-libvpx --extra-cflags="-fexceptions" && make'
                returncode = call(cmd, shell=True, cwd="libs/"+self.ffmpeg)
                if returncode != 0:
                     self.error("Could not build ffmpeg-webM")
                 
        ## Crcanvas
        if options.gui:
            cmd = "chmod 777 configure && ./configure && make"
            self.msg("*** BUILDING CRCANVAS ***\n" + cmd)
            returncode = call(cmd, shell=True, cwd="libs/"+self.crcanvas)
            if returncode != 0:
                self.error("Could not build crcanvas")
                
        ## Jrtplib
        if options.jrtplib:
            cmd = "cmake --disable-jthread --disable-memory . && make"
            self.msg("*** BUILDING JRTPLIB ***\n" + cmd)
            returncode = call(cmd, shell=True, cwd="libs/"+self.jrtp)
            if returncode != 0:
                self.error("Could not build jrtplib")
        
        ## Configure
        cmd = "autoreconf -i"
        self.msg("*** CREATING CONFIGURE ***\n" + cmd)
        returncode = call(cmd, shell=True, cwd=".")
        if returncode != 0:
            self.error("Could not create configure")

if __name__ == "__main__":
    main().main(*sys.argv[1:])