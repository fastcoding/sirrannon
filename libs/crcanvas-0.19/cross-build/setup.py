""" crcanvas: A GTK/Cairo Canvas Widget

This is a specially created python wrapper only WIN32 installable 
version of cranvas.
"""

import distutils
import distutils.core
from distutils.core import setup
from distutils.core import Extension

doclines = __doc__.split("\n")

setup(
        name = "crcanvas", 
        version = "0.12", 
        url="http://geocanvas.sourceforge.net/crcanvas/index.html",
        maintainer="Robert Gibbs",
        maintainer_email="bgibbs@sourceforge.net",
        description = doclines[0],
        long_description = "\n".join(doclines[2:]),
        data_files = [
        ("Lib/site-packages", [
            "crcanvas.pth"
            ]),
        ("Lib/site-packages/crcanvas", [
            "../python/.libs/crcanvas.dll", 
            "../crcanvas/.libs/libcrcanvas-0.dll"
            ]),
        ("crcanvas-examples", [
            "../python/custom-demo.py",
            "../python/mvc-demo.py",
            ])
        ]
        )
