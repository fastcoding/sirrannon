## Sirannon binary
bin_PROGRAMS = sirannon

## Sirannon Library
lib_LTLIBRARIES =
if BUILD_LIBRARY
lib_LTLIBRARIES += libsirannon.la
endif

## Includes
libsirannoninclude_HEADERS = sirannon.h \
	Core/MediaProcessor.h Core/MediaPacket.h Core/MediaDescriptor.h Core/SirannonTime.h Core/ProcessorManager.h \
	Core/MediaProcessor_priv.h Core/MediaPacket_priv.h Core/ProcessorManager_priv.h Core/SirannonTime_priv.h Core/SirannonException.h \
	Common/Boost.h
libsirannonincludedir = ${includedir}/sirannon

## Sources
sirannon_SOURCES =
nodist_sirannon_SOURCES = 

## PRIVATE
if PRIVATE
nodist_sirannon_SOURCES += Private/AQUAServer.cpp Private/DIORSession.h Private/AQUAServer.h Private/AQUAWriter.cpp Private/DIORServer.cpp Private/DIORServer.h Private/AQUASession.cpp Private/AQUAMonitor.h Private/AQUAMonitor.cpp Private/DIORSession.cpp Private/AQUAWriter.h Private/AQUASession.h 
endif

## LOCAL
if LOCAL
nodist_sirannon_SOURCES += Local/test.h Local/IConcealClassifier.cpp Local/IConcealClassifier.h Local/test.cpp Local/Telefoonlijst_240809.xls 
endif

## High-Level
sirannon_SOURCES += \
	HighLevel/Proxy.cpp HighLevel/Proxy.h \
	HighLevel/FileStreamer.cpp HighLevel/FileStreamer.h \
	HighLevel/TranscoderSession.cpp HighLevel/TranscoderSession.h \
	HighLevel/ProxyStreamer.cpp HighLevel/ProxyStreamer.h \
	HighLevel/Segmenter.cpp HighLevel/Segmenter.h

## Reader
sirannon_SOURCES += \
	Reader/Reader.cpp Reader/Reader.h \
	Reader/FFmpegReader.cpp Reader/FFmpegReader.h \
	Reader/AVCReader.cpp Reader/AVCReader.h \
	Reader/BasicReader.cpp Reader/BasicReader.h \
	Reader/LiveReader.cpp Reader/LiveReader.h \
	Reader/AMR_WBP_Reader.cpp Reader/AMR_WBP_Reader.h \
	Reader/FakeReader.cpp Reader/FakeReader.h

## Packetizer
sirannon_SOURCES += \
	Packetizer/Packetizer.cpp Packetizer/Packetizer.h \
	Packetizer/Unpacketizer.cpp Packetizer/Unpacketizer.h \
	Packetizer/PESPacketizer.cpp Packetizer/PESPacketizer.h \
	Packetizer/MP2Packetizer.cpp Packetizer/MP2Packetizer.h \
	Packetizer/MP4Packetizer.cpp Packetizer/MP4Packetizer.h \
	Packetizer/AVCPacketizer.cpp Packetizer/AVCPacketizer.h \
	Packetizer/AMRPacketizer.cpp Packetizer/AMRPacketizer.h \
	Packetizer/AMR_WBP_Packetizer.cpp Packetizer/AMR_WBP_Packetizer.h \
	Packetizer/AC3Packetizer.cpp Packetizer/AC3Packetizer.h \
	Packetizer/SirannonPacketizer.cpp Packetizer/SirannonPacketizer.h \
	Packetizer/AVCUnpacketizer.cpp Packetizer/AVCUnpacketizer.h \
	Packetizer/MP4Unpacketizer.cpp Packetizer/MP4Unpacketizer.h \
	Packetizer/PESUnpacketizer.cpp Packetizer/PESUnpacketizer.h \
	Packetizer/MP2Unpacketizer.cpp Packetizer/MP2Unpacketizer.h	\
	Packetizer/AMRUnpacketizer.cpp Packetizer/AMRUnpacketizer.h	\
	Packetizer/DefaultPacketizer.cpp Packetizer/DefaultPacketizer.h
	
## Multiplexer
sirannon_SOURCES += \
	Multiplexer/Multiplexer.cpp Multiplexer/Multiplexer.h \
	Multiplexer/TSMultiplexer.cpp Multiplexer/TSMultiplexer.h \
	Multiplexer/TSDemultiplexer.cpp Multiplexer/TSDemultiplexer.h \
	Multiplexer/FFmpegMultiplexer.cpp Multiplexer/FFmpegMultiplexer.h \
	Multiplexer/FFmpegDemultiplexer.cpp Multiplexer/FFmpegDemultiplexer.h
	
## Scheduler
sirannon_SOURCES += Scheduler/Scheduler.cpp Scheduler/Scheduler.h

## Communicator
sirannon_SOURCES += \
	Communicator/Communicator.cpp Communicator/Communicator.h \
	Communicator/MediaServer.cpp Communicator/MediaServer.h \
	Communicator/MediaSession.cpp Communicator/MediaSession.h \
	Communicator/RTPTransmitter.cpp Communicator/RTPTransmitter.h \
	Communicator/RTPReceiver.cpp Communicator/RTPReceiver.h \
	Communicator/UDPTransmitter.cpp Communicator/UDPTransmitter.h \
	Communicator/UDPReceiver.cpp Communicator/UDPReceiver.h \
	Communicator/TCPTransmitter.cpp Communicator/TCPTransmitter.h \
	Communicator/TCPReceiver.cpp Communicator/TCPReceiver.h \
	Communicator/GigabitTransmitter.cpp Communicator/GigabitTransmitter.h
	
## HTTP
sirannon_SOURCES += \
	Communicator/HTTP/HTTPConnection.cpp Communicator/HTTP/HTTPConnection.h \
	Communicator/HTTP/HTTPServer.cpp Communicator/HTTP/HTTPServer.h \
	Communicator/HTTP/HTTPSession.cpp Communicator/HTTP/HTTPSession.h \
	Communicator/HTTP/HTTPClient.cpp Communicator/HTTP/HTTPClient.h
	
sirannon_SOURCES += \
	Communicator/RTSP/RTSPSession.cpp Communicator/RTSP/RTSPSession.h \
	Communicator/RTSP/RTSPServer.cpp Communicator/RTSP/RTSPServer.h \
	Communicator/RTSP/RTSPClient.cpp Communicator/RTSP/RTSPClient.h
	
## RTMP
sirannon_SOURCES += \
	Communicator/RTMP/RTMP_AMF.cpp Communicator/RTMP/RTMP_AMF.h \
	Communicator/RTMP/RTMPHandshake.cpp Communicator/RTMP/RTMPHandshake.h \
	Communicator/RTMP/RTMPChunkStream.cpp Communicator/RTMP/RTMPChunkStream.h \
	Communicator/RTMP/RTMPSession.cpp Communicator/RTMP/RTMPSession.h \
	Communicator/RTMP/RTMPTConnection.cpp Communicator/RTMP/RTMPTConnection.h \
	Communicator/RTMP/RTMPServer.cpp Communicator/RTMP/RTMPServer.h \
	Communicator/RTMP/RTMPTServer.cpp Communicator/RTMP/RTMPTServer.h \
	Communicator/RTMP/RTMPPacketizer.cpp Communicator/RTMP/RTMPPacketizer.h \
	Communicator/RTMP/RTMPClient.cpp Communicator/RTMP/RTMPClient.h \
	Communicator/RTMP/RTMPTClient.cpp Communicator/RTMP/RTMPTClient.h
	
## Classifier
sirannon_SOURCES += \
	Classifier/Classifier.cpp Classifier/Classifier.h \
	Classifier/SVCClassifier.cpp Classifier/SVCClassifier.h
	
## Writer
sirannon_SOURCES += \
	Writer/Writer.cpp Writer/Writer.h \
	Writer/FFmpegWriter.cpp Writer/FFmpegWriter.h

## Transformation
sirannon_SOURCES += \
	Transform/FrameTransformer.cpp Transform/FrameTransformer.h \
	Transform/Transcoder.cpp Transform/Transcoder.h \
	Transform/TranscoderAudio.cpp Transform/TranscoderAudio.h \
	Transform/FFmpegDecoder.cpp Transform/FFmpegDecoder.h
	
## Misc
sirannon_SOURCES += \
	Misc/FrameTagger.cpp Misc/FrameTagger.h \
	Misc/Logger.cpp Misc/Logger.h \
	Misc/Statistics.cpp Misc/Statistics.h \
	Misc/Example.cpp Misc/Example.h \
	Misc/GOPSplitter.cpp Misc/GOPSplitter.h \
	Misc/PcapWriter.cpp Misc/PcapWriter.h \
	Misc/YUVDisplay.cpp Misc/YUVDisplay.h \
	Misc/Restamp.cpp Misc/Restamp.h
		
## Common
sirannon_SOURCES += \
	Common/Boost.h Common/Boost.cpp \
	Common/crc.h Common/Endianness.h \
	Common/ffmpeg.cpp Common/ffmpeg.h \
	Common/h264_avc.cpp Common/h264_avc.h Common/h264_avc_priv.h \
	Common/sha2.cpp Common/sha2.h Common/hmac_sha2.cpp Common/hmac_sha2.h \
	Common/PortManager.cpp Common/PortManager.h \
	Common/FileManager.cpp Common/FileManager.h \
	Common/RandomGenerator.cpp Common/RandomGenerator.h \
	Common/Utils.cpp Common/Utils.h \
	Common/Frame.cpp Common/Frame.h \
	Common/sdp.cpp Common/sdp.h \
	Common/SirannonSocket.cpp Common/SirannonSocket.h \
	Common/Flash.cpp Common/Flash.h \
	Common/Bits.cpp Common/Bits.h Common/Bits_priv.h \
	Common/Url.cpp Common/Url.h \
	Common/Mpeg.h \
	Common/Base64.cpp Common/Base64.h \
	Common/Network.h \
	Common/IPAddress.cpp Common/IPAddress.h \
    Common/str.h
	
## Core
sirannon_SOURCES += \
	Core/SirannonException.cpp Core/SirannonException.h \
	Core/SirannonTime.cpp Core/SirannonTime.h Core/SirannonTime_priv.h \
	Core/XMLStream.cpp Core/XMLStream.h \
	Core/Block.cpp Core/Block.h \
	Core/System.cpp Core/System.h \
	Core/OSSupport.cpp Core/OSSupport.h \
	Core/ConnectionInterface.cpp Core/ConnectionInterface.h \
	Core/Link.cpp Core/Link.h \
	Core/MediaPacket.cpp Core/MediaPacket.h Core/MediaPacket_priv.h \
	Core/MediaDescriptor.cpp Core/MediaDescriptor.h \
	Core/ProcessorManager.cpp Core/ProcessorManager.h Core/ProcessorManager_priv.h \
	Core/MemoryManager.cpp Core/MemoryManager.h \
	Core/SirannonPrint.cpp Core/SirannonPrint.h \
	Core/Interfaces.h sirannon.h	

## Put MediaProcessor in the extra sources so the object file is listed last when linking
EXTRA_sirannon_SOURCES = Core/MediaProcessor.cpp Core/MediaProcessor.h Core/MediaProcessor_priv.h

## Library sources
libsirannon_la_SOURCES = $(sirannon_SOURCES)
nodist_libsirannon_la_SOURCES = $(nodist_sirannon_SOURCES)
EXTRA_libsirannon_la_SOURCES = $(EXTRA_sirannon_SOURCES)

## Executable
sirannon_SOURCES += sirannon.cpp

## Force xstremer-sirannon.o to be rebuilt every run
sirannon-sirannon.$(OBJEXT): $(sirannon_SOURCES)

## sirannon flags
sirannon_LDADD = sirannon-MediaProcessor.o
sirannon_LDFLAGS = 
sirannon_CXXFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-reorder -Wno-sign-compare
sirannon_CPPFLAGS = -I$(top_srcdir)/src/Common -I$(top_srcdir)/src/Core -DSVN_VERSION=\"$(SVN_VERSION)\" -D_FILE_OFFSET_BITS=64
libsirannon_la_LDFLAGS =
libsirannon_la_CXXFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-reorder -Wno-sign-compare
libsirannon_la_CPPFLAGS = -I$(top_srcdir)/src/Common -I$(top_srcdir)/src/Core -DSVN_VERSION=\"$(SVN_VERSION)\"
