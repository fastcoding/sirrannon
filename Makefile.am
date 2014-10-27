ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = src/Local \
	doc/manual/manual.pdf doc/manual/manual.tex doc/manual/doc-gen.tex doc/manual/images \
	doc/papers/demo/demo.pdf libs python sirannon.py \
	dat/xml/demo1.xml dat/xml/demo2.xml dat/xml/demo3.xml dat/xml/demo4A.xml dat/xml/demo4B.xml \
	dat/xml/demo5A.xml dat/xml/demo5B.xml dat/xml/bandwidth.xml \
	dat/xml/example1.xml dat/xml/example2.xml dat/xml/example3.xml dat/xml/example4.xml \
	dat/xml/example5.xml dat/xml/example6.xml dat/xml/example7A.xml dat/xml/example7B.xml \
	dat/xml/example8.xml dat/xml/tutorial.xml dat/xml/streamer.xml \
	dat/xml/extractor-video.xml dat/xml/extractor-audio.xml dat/xml/info.xml \
	dat/xml/media-server.xml dat/xml/media-server-std.xml
SUBDIRS = src

dist-hook:
	rm -rf `find $(distdir) -name .libs`
	rm -rf `find $(distdir) -name .deps`
	rm -rf `find $(distdir) -name .svn`
	rm -rf `find $(distdir) -name *.sdp`
	rm -rf `find $(distdir) -name *.o`
	rm -rf `find $(distdir) -name *.lo`
	rm -rf `find $(distdir) -name *.a`
	rm -rf `find $(distdir) -name *.la`
	rm -rf `find $(distdir) -name *.d`
	rm -rf `find $(distdir) -name *.exe`
	rm -rf `find $(distdir) -name *.pyc`
	mv $(distdir)/doc/papers/demo/demo.pdf $(distdir)/doc/demo.pdf
	rmdir $(distdir)/doc/papers/demo
	rmdir $(distdir)/doc/papers
	chmod a+x $(distdir)/python/local.py
	chmod a+x $(distdir)/python/doc.py	
	chmod a+x $(distdir)/python/bootstrap.py
	chmod a+x $(distdir)/python/stats.py
	chmod a+x $(distdir)/configure
	chmod a+x $(distdir)/sirannon.py
	chmod a+x $(distdir)/python/oldGUI/sirannon.py
	chmod a+x $(distdir)/python/newGUI/sirannon.py
