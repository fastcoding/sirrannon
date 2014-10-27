#!/usr/bin/env python
import sys
import os
import subprocess

class main:
    def __init__(self):
        pass
    
    def main(self, *args):
        ## Reset the cwd         
        os.chdir(args[0])
        
        ## All files in src/Local
        lLocal = [s for s in os.listdir("src/Private") if s[0] != '.']
        sLocal = reduce(lambda x,y: "".join( (x, "Private/", y , " ") ), lLocal, "")
        
        ## Add this in src/Automake
        oFile1 = open("src/Makefile.am", "r")
        sFile = oFile1.read()
        oFile1.close()
        idx1 = sFile.find("## PRIVATE")
        idx2 = sFile.find("## LOCAL")
        if idx1 < 0 or idx2 < 0:
            raise RuntimeError("Corrupted src/Makefile.am")
        sCode = "## PRIVATE\nif PRIVATE\nnodist_sirannon_SOURCES += %s\nendif\n\n" % sLocal
        sFile = sFile[:idx1] + sCode + sFile[idx2:]
        oFile2 = open("src/Makefile.am", "w")
        oFile2.write(sFile)
        oFile2.close()
    
if __name__ == "__main__":
    main().main(*sys.argv[1:])