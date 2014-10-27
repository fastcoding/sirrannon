#!/usr/bin/env python
from python.newGUI.sirannon import SirannonGTK

if __name__ == '__main__':
    import sys
    gui = SirannonGTK()
    gui.main(sys.argv[1:])