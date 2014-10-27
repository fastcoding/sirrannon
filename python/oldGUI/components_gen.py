from sirannon import *
g_dComponents     = {}
g_dBaseComponents = {}
class core(Component):
	def __init__(self,sType,sName):
		Component.__init__(self,sType,sName)
		self.SetTip("")
		self.AddParamater("debug", "bool", "false", "if true, print debug info for this component")
		self.AddParamater("thread", "bool", "false", "if true, run the component in a seperate thread")
g_dBaseComponents["core"] = core

class packetizer(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
		self.AddParamater("tracefile", "string", "", "if defined, the path of trace where to log information about the split packets")
		self.AddParamater("trace-pts", "bool", "false", "if true, write the PTS instead of the DTS in the log file")
g_dBaseComponents["packetizer"] = packetizer

class AC3_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"AC3-packetizer",sName)
		self.SetTip("Packetizes AC3 audio frames into packets suitable for RTP as defined in (draft) RFC 4184.")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of a network packet")
		self.AddParamater("draft", "bool", "true", "if true, fill out the header according to the draft version of RFC 4184; this is the version supported by live555/vlc/mplayer")
g_dComponents["AC3-packetizer"] = AC3_packetizer

class AMR_WBP_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"AMR-WBP-packetizer",sName)
		self.SetTip("AMR-WB+ packets are aggregated into packets of (near) MTU size according to RFC 4352")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of a network packet")
g_dComponents["AMR-WBP-packetizer"] = AMR_WBP_packetizer

class AMR_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"AMR-packetizer",sName)
		self.SetTip("AMR and AMR-WB packets are aggregated into packets of (near) MTU size according to RFC 3267")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of a network packet")
		self.AddParamater("maxptime", "int", "200", "in milliseconds, if >= 0, the maximum duration of an aggregate frame, if < 0, omit this requirement")
g_dComponents["AMR-packetizer"] = AMR_packetizer

class AVC_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"AVC-packetizer",sName)
		self.SetTip("Packetizes H264 frames into packets suitable for the network as defined in RFC 3984.")
		self.AddParamater("iMTU", "int", "1500", "in bytes, the maximum size of a network packet")
		self.AddParamater("aggregate", "bool", "false", "if true, aggregate small packets into one network packet")
g_dComponents["AVC-packetizer"] = AVC_packetizer

class MP2_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"MP2-packetizer",sName)
		self.SetTip("Packetizes MPEG1&2 audio and video frames into packets suitable for the network as defined in RFC 2250.")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of a network packet")
g_dComponents["MP2-packetizer"] = MP2_packetizer

class MP4_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"MP4-packetizer",sName)
		self.SetTip("Packetizes MPEG4 audio and video frames into packets suitable for the network as defined in RFC 3640.")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of a network packet")
		self.AddParamater("aggregate", "bool", "false", "if true, aggregate small packets into one network packet")
g_dComponents["MP4-packetizer"] = MP4_packetizer

class PES_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"PES-packetizer",sName)
		self.SetTip("PES packetization is the required format for transport streams TS-multiplexer. All packets belonging to one frame are aggregated into one PES-packet. If the total size of the frame is larger than 65500 bytes two or more PES-packets are generated unless the option zero-length is set.")
		self.AddParamater("delta", "int", "0", "in ms, the amount of time to add to DTS/PTS to make sure is substractable by a PCR")
		self.AddParamater("insert-AUD", "bool", "false", "if true, add AUDs before each H.264 frame")
		self.AddParamater("audio-per-PES", "int", "1", "number of audio frames per PES-packet")
		self.AddParamater("zero-length", "bool", "true", "video only, if true, set 0 as PES-packet-length and generate a single PES-packet per frame")
g_dComponents["PES-packetizer"] = PES_packetizer

class RTMP_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"RTMP-packetizer",sName)
		self.SetTip("This packetizer produces an RTMP chunk stream from the ingested MediaPackets. Such a stream can directly be sent to a Flash Media Server. Unpacketization of this stream is done by the component RMTP-client internally.")
		self.AddParamater("chunk-ID", "int", "5", "the identifier for this RTMP chunk stream")
		self.AddParamater("stream-ID", "int", "1", "the identifier to which global stream this RTMP chunk stream belongs")
		self.AddParamater("chunk-size", "int", "128", "in bytes, the maximum size of each RTMP chunk")
g_dComponents["RTMP-packetizer"] = RTMP_packetizer

class default_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"default-packetizer",sName)
		self.SetTip("For each new stream creates at runtime a fitting packetizer depending on the codec or the parameter type.")
		self.AddParamater("type", "string", "", "if defined, force every packetizer to be of this type, useful for forcing PES-packetizer for example")
g_dComponents["default-packetizer"] = default_packetizer

class sirannon_packetizer(packetizer):
	def __init__(self,sName):
		packetizer.__init__(self,"sirannon-packetizer",sName)
		self.SetTip("This simple and generic packetizer can handle any content. However, this packetization is internal to Sirannon and is not recognized by other players. Use sirannon-unpacketizer to unpacketize this stream again.")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of a network packet")
g_dComponents["sirannon-packetizer"] = sirannon_packetizer

class writer(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
g_dBaseComponents["writer"] = writer

class basic_writer(writer):
	def __init__(self,sName):
		writer.__init__(self,"basic-writer",sName)
		self.SetTip("Writes the content of each received packet to a file and deletes the packet. When an end-packet is received, the file is closed.")
		self.AddParamater("filename", "string", "", "the path of the file where to write to")
		self.AddParamater("flush", "bool", "false", "if true, flush the IO buffer after each write")
		self.AddParamater("fragmented", "bool", "false", "if true, after each reset, close the container and open a new container with a name of the form e.g. demo-0.avi, demo-1.avi, demo-2.avi, etc.")
		self.AddParamater("complete", "bool", "false", "if true, if the file is not closed when entering the destructor, delete the file")
g_dComponents["basic-writer"] = basic_writer

class ffmpeg_writer(writer):
	def __init__(self,sName):
		writer.__init__(self,"ffmpeg-writer",sName)
		self.SetTip("Joins packets from different source into a container format supported by FFMPEG (eg. FLV, WEBM, MP4). The container is written to a file.")
		self.AddParamater("filename", "string", "", "the path of the file where to write to")
		self.AddParamater("initial-delay", "int", "0", "in ms, the minimal delay between the first received packet and first multiplexed packet")
		self.AddParamater("delay", "int", "1000", "in ms, the minimal amount of data present for each stream of the multiplex before the next packet is written")
		self.AddParamater("streams", "int", "-1", "the number of different streams required before multiplexing, -1 omits this requirement")
		self.AddParamater("format", "string", "", "if defined, overrules the format determined by the extension in the filename")
		self.AddParamater("fragmented", "bool", "false", "if true, after each reset, close the container and open a new container with a name of the form e.g. demo-0.avi, demo-1.avi, demo-2.avi, etc.")
g_dComponents["ffmpeg-writer"] = ffmpeg_writer

class media_client(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Media Clients connect to servers and retrieve a stream using a particular protocol. The component outputs frames just as would a a Reader component. In this way streams can be requested and captured (using for example \textit{FFMPEG-writer} or \textit{writer}) or resent using a different protocol (for example converting an RTMP stream to a HTTP stream). Proxy applications for Media Servers (RTMP-proxy, RTMPT-proxy, RTSP-proxy, HTTP-proxy) use this feature.")
		self.AddParamater("url", "string", "", "the url to retrieve")
		self.AddParamater("auto-play", "bool", "true", "if true, instantly play the stream")
g_dBaseComponents["media-client"] = media_client

class HTTP_capture(media_client):
	def __init__(self,sName):
		media_client.__init__(self,"HTTP-capture",sName)
		self.SetTip("Requests and receives a stream via HTTP. Keeps the container and generates chunks from it.")
		self.AddParamater("chunk-size", "int", "65536", "in bytes, the size of the chunks from the container to generate")
g_dComponents["HTTP-capture"] = HTTP_capture

class HTTP_client(media_client):
	def __init__(self,sName):
		media_client.__init__(self,"HTTP-client",sName)
		self.SetTip("Requests and receives a stream via HTTP. Demultiplexes the container and generates frames.")
		self.AddParamater("format", "string", "", "the format of the container, if not defined, guess the format based on the extension in the URL")
		self.AddParamater("M3U", "bool", "false", "if true, the URL links to an extended M3U file used by Apple Live HTTP Streaming. The URLs contained in the M3U response will be contacted in turn.")
g_dComponents["HTTP-client"] = HTTP_client

class RTMP_client(media_client):
	def __init__(self,sName):
		media_client.__init__(self,"RTMP-client",sName)
		self.SetTip("Requests and receives a stream via RTMP. Generates audio and video frames.")
		self.AddParamater("mov-frame", "bool", "false", "if true, keep the frames in MOV/MP4 structure for AVC and AAC")
		self.AddParamater("chunk-size", "int", "65536", "in bytes, the size of the chunks from the container to generate")
g_dComponents["RTMP-client"] = RTMP_client

class RTMPT_client(media_client):
	def __init__(self,sName):
		media_client.__init__(self,"RTMPT-client",sName)
		self.SetTip("Requests and receives a stream via RTMPT. Generates audio and video frames.")
		self.AddParamater("polling", "int", "1000", "in ms, the time interval between two successive HTTP POSTs during play")
g_dComponents["RTMPT-client"] = RTMPT_client

class RTSP_client(media_client):
	def __init__(self,sName):
		media_client.__init__(self,"RTSP-client",sName)
		self.SetTip("Requests and receives a stream via RTSP. Generates audio and video frames.")
g_dComponents["RTSP-client"] = RTSP_client

class demultiplexer(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
g_dBaseComponents["demultiplexer"] = demultiplexer

class FFMPEG_demultiplexer(demultiplexer):
	def __init__(self,sName):
		demultiplexer.__init__(self,"FFMPEG-demultiplexer",sName)
		self.SetTip("This component is identical to the component FFMPEG-reader, with the exception that the source is not a file but a stream of chunks from a container.")
		self.AddParamater("format", "string", "flv", "container format of the input")
		self.AddParamater("chunk-size", "int", "32768", "amount of bytes fed to the FFMPEG IOContext each time")
		self.AddParamater("loop", "int", "1", "the number of times to play the video, -1 being infinite, 0 interpreted as 1")
		self.AddParamater("videoroute", "int", "100", "the xroute that will be assigned to packets containing video")
		self.AddParamater("audioroute", "int", "200", "the xroute that will be assigned to packets containing audio")
		self.AddParamater("dts-start", "int", "1000", "in ms, the timestamp that will be added to the DTS & PTS of each frame")
		self.AddParamater("video-mode", "bool", "true", "if true, video will be read from the container, if false video will be ignored")
		self.AddParamater("audio-mode", "bool", "true", "if true, audio will be read from the container, if false audio will be ignored")
		self.AddParamater("seek", "int", "-1", "in ms, the timestamp to jump to into the stream, -1 implying no seek")
		self.AddParamater("dts-start", "int", "1000", "in ms, specifies the value of the timestamp of the first frame")
		self.AddParamater("add-parameter-sets", "bool", "true", "H.264/AVC only, if true, extract the parameter sets from the container and insert them into the stream")
		self.AddParamater("repeat-parameter-sets", "bool", "false", "H.264/AVC only, if true, repeat the parameter sets before each IDR frame")
		self.AddParamater("mov-frame", "bool", "false", "MOV/MP4/F4V container only, if true, keep frames in the format of the container, as opposed to annex-B H.264/AVC streams with start codes before each NAL unit")
		self.AddParamater("skip-SEI", "bool", "false", "if true, remove SEI NALUs from the stream")
		self.AddParamater("skip-AUD", "bool", "false", "if true, remove AUD NALUs from the stream")
g_dComponents["FFMPEG-demultiplexer"] = FFMPEG_demultiplexer

class TS_demultiplexer(demultiplexer):
	def __init__(self,sName):
		demultiplexer.__init__(self,"TS-demultiplexer",sName)
		self.SetTip("Unmultiplexes an MPEG Transport Stream into the original streams each consisting of series of PES-packets. It performs the reverse operation of ts-multiplexer. The MPEG Transport Stream can be as large as entire multi-channel stream.")
		self.AddParamater("channel", "int", "-1", "the selected channel to extract, -1 being all channels")
		self.AddParamater("video-route", "int", "100", "the xroute that will be assigned to packets containing video")
		self.AddParamater("audio-route", "int", "100", "the xroute that will be assigned to packets containing audio")
g_dComponents["TS-demultiplexer"] = TS_demultiplexer

class multiplexer(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Multiplexers buffer MediaPackets coming from different sources and multiplex those based on the DTS of each MediaPacket. Unmultiplexers perform the reverse operation and restore the original streams.")
		self.AddParamater("delay", "int", "1000", "in ms, the minimal amount of data present for each stream of the multiplex before the next packet is released")
		self.AddParamater("streams", "int", "-1", "the number of different streams required before multiplexing, -1 omits this requirement")
g_dBaseComponents["multiplexer"] = multiplexer

class FFMPEG_multiplexer(multiplexer):
	def __init__(self,sName):
		multiplexer.__init__(self,"FFMPEG-multiplexer",sName)
		self.SetTip("Joins packets from different source into a container (no file!) supported by FFMPEG (eg. FLV, WEBM, MP4). The container is released in a stream of MediaPackets containing chunks of the container.")
		self.AddParamater("chunk-size", "int", "262144", "in bytes, the maximum size of each chunk")
		self.AddParamater("format", "string", "flv", "format of the container given as file extension (eg. flv, webm, mov, mp4, avi)")
		self.AddParamater("streamed", "bool", "false", "if true, disables seeking backwards into the generated chunks")
g_dComponents["FFMPEG-multiplexer"] = FFMPEG_multiplexer

class TS_multiplexer(multiplexer):
	def __init__(self,sName):
		multiplexer.__init__(self,"TS-multiplexer",sName)
		self.SetTip("Multiplexes audio and video into an MPEG Transport Stream (TS). CAVEAT: If the video or audio come from different readers, you must set the parameter streams or initial-delay, or the component will throw an error.")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum size of an aggregated packet, typically for an iMTU of 1500 it means 7 Transport Stream packets (1370 bytes)")
		self.AddParamater("aggregate", "bool", "true", "if true, several transport stream packets will be aggregated until their size would exceed the iMTU")
		self.AddParamater("shape", "int", "200", "in ms, the amount of media to multiplex each pass, this value overwrites the value of delay")
		self.AddParamater("pcr-delay", "int", "400", "in ms, how much the PCR is in advance of the DTS, the pcr-delay must be lower than the initial DTS of the reader component")
		self.AddParamater("mux-rate", "int", "-1", "in kbps, if -1, no fixed multiplexing rate, if 0, the component will guess the mux rate based on the bitrate of the input, if > 0, use this value as mux-rate")
		self.AddParamater("continuous-timestamps", "bool", "true", "if true, use the PCR clock as timestamp for the transport stream, if false, use the timestamps of the PCR stream as timestamps for the transport stream")
		self.AddParamater("interleave", "bool", "true", "if false, all transport stream packets belonging to one frame from a PID are consecutive, if true, they are interleaved with transport stream packets from other PIDs (e.g. audio)")
		self.AddParamater("psi-on-key", "bool", "false", "if true, generate an extra PSI triplet (SDT, PAT and PMT) before the start of every key frame of the PCR stream")
g_dComponents["TS-multiplexer"] = TS_multiplexer

class std_multiplexer(multiplexer):
	def __init__(self,sName):
		multiplexer.__init__(self,"std-multiplexer",sName)
		self.SetTip("Joins packets from different sources by only ensuring that the DTS and other timing information rise monotonely. This is a requirement for many protocols and containers.")
g_dComponents["std-multiplexer"] = std_multiplexer

class unit_multiplexer(multiplexer):
	def __init__(self,sName):
		multiplexer.__init__(self,"unit-multiplexer",sName)
		self.SetTip("Joins packets from different sources by only ensuring that the unitnumber rises monotonely.")
g_dComponents["unit-multiplexer"] = unit_multiplexer

class receiver(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Receivers provide the interface from the network for the following protocols: UDP, TCP and RTP/UDP.")
		self.AddParamater("port", "int", "5000", "reception port")
		self.AddParamater("video-route", "int", "100", "the xroute that will be assigned to packets containing video")
		self.AddParamater("audio-route", "int", "200", "the xroute that will be assigned to packets containing audio")
		self.AddParamater("buffer", "int", "0", "if >0, the size of the protocol buffer, your OS must still accept this value, check 'UDP Buffer Sizing' in Google for more information")
		self.AddParamater("extension", "bool", "false", "if true, the additional sirannon header is parsed from the packet. Caveat, if the header was not present the stream will be corrupted except for RTP. Conversely, if the header was present and this value is false, the stream will be corrupted except for RTP")
		self.AddParamater("multicast", "bool", "false", "if true, join a multicast address (not for TCP)")
		self.AddParamater("multicast-server", "string", "", "multicast address (not for TCP)")
g_dBaseComponents["receiver"] = receiver

class RTP_receiver(receiver):
	def __init__(self,sName):
		receiver.__init__(self,"RTP-receiver",sName)
		self.SetTip("Receives RTP streams using the open source library jrtplib. RTCP packets are automatically generated. The additional header is parsed from the RTP header extension if present.")
		self.AddParamater("tracefile", "string", "", "if defined, the path of the trace where to log information about the received packets")
		self.AddParamater("buffer", "int", "8388608", "in bytes, the size of the underlyinhg UDP buffer, increase this value when receiving high bitrate streams, make sure yours OS accepts such large values (see \url{http://www.29west.com/docs/THPM/udp-buffer-sizing.html})")
		self.AddParamater("hash-file", "string", "", "if defined, the path of a file in which the content of a header extension with ID(EXT-HASH: 0xB) is written")
g_dComponents["RTP-receiver"] = RTP_receiver

class TCP_receiver(receiver):
	def __init__(self,sName):
		receiver.__init__(self,"TCP-receiver",sName)
		self.SetTip("Provides a non-blocking TCP socket from the network.")
		self.AddParamater("connect", "bool", "false", "if true, connect to the server, if false, listen for an incoming connection")
g_dComponents["TCP-receiver"] = TCP_receiver

class UDP_receiver(receiver):
	def __init__(self,sName):
		receiver.__init__(self,"UDP-receiver",sName)
		self.SetTip("Provides a non-blocking UDP socket from the network.")
g_dComponents["UDP-receiver"] = UDP_receiver

class miscellaneous(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
g_dBaseComponents["miscellaneous"] = miscellaneous

class GOP_splitter(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"GOP-splitter",sName)
		self.SetTip("Cyclically classifies each GOP by increasing the original xroute, so that xroute cycles through [route,route+split[")
		self.AddParamater("split", "int", "1", "in how many parts to split the stream")
		self.AddParamater("sync", "bool", "false", "if true, drop all frames until the first PPS/SPS/IDR packet")
g_dComponents["GOP-splitter"] = GOP_splitter

class PCAP_writer(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"PCAP-writer",sName)
		self.SetTip("Captures packets using a filter and saves those to a file. Uses libpcap for capturing. Needs to have permission to access the interfaces (eg. sudo).")
		self.AddParamater("interface", "string", "eth0", "the name of device on which to capture")
		self.AddParamater("filename", "string", "", "the path of the file where to write the captured packets")
		self.AddParamater("filter", "string", "", "if defined, overwrite the built-in filter")
		self.AddParamater("port", "int", "1234", "the UDP port on which to listen for traffic (when using the built-in filter)")
		self.AddParamater("bitrate", "int", "20", "in Mbps, estimated bitrate of stream which determines the underlying buffer size. When unsure of the bitrate, use a royal upper limit. If the debug reports dropped packets, increase this value.")
g_dComponents["PCAP-writer"] = PCAP_writer

class YUV_display(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"YUV-display",sName)
		self.SetTip("Creates a rudimentary video player with no user controls save 'ESC' (which forcibly terminates the video). This component displays immediately any YUV frame it receives. Sirannon must be compiled with the option --with-libSDL to use this component.")
		self.AddParamater("width", "int", "-1", "in pixels, if defined, the width of the video display, otherwise obtain the information from the MediaPackets containing the YUV frames (pPckt->desc->width)")
		self.AddParamater("height", "int", "-1", "in pixels, if defined, the width of the video display, otherwise obtain the information from the MediaPackets containing the YUV frames (pPckt->desc->height)")
		self.AddParamater("full-screen", "bool", "false", "if true, display the video full screen")
g_dComponents["YUV-display"] = YUV_display

class example(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"example",sName)
		self.SetTip("Example component demonstrating the basic API of a functional component")
		self.AddParamater("test", "int", "5", "input variable")
		self.AddParamater("foo", "bool", "false", "an example flag")
g_dComponents["example"] = example

class fake_reader(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"fake-reader",sName)
		self.SetTip("Generates random YUV frames with specified dimensions and/or targetted bitrate.")
		self.AddParamater("width", "int", "1920", "the width of the fake frame")
		self.AddParamater("height", "int", "1080", "the height of the fake frame")
		self.AddParamater("bits-per-pixel", "int", "12", "the number of bits per pixel of the fake frame")
		self.AddParamater("fps", "double", "25.", "the number of frames per second")
		self.AddParamater("bitrate", "int", "-1", "if defined, overwrite the bitrate implied by the combination of fps and frame dimensions")
		self.AddParamater("duration", "int", "-1", "in ms, if defined the duration of the sequence, otherwise generate frames indefinately")
		self.AddParamater("mtu", "int", "-1", "if > 0, divide each frame into packets of maximum mtu size")
g_dComponents["fake-reader"] = fake_reader

class frame_analyzer(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"frame-analyzer",sName)
		self.SetTip("This component parses elementary MPEG-1, MPEG-2, MPEG-4 or H.264/AVC streams to determine the frame type.")
g_dComponents["frame-analyzer"] = frame_analyzer

class gigabit_transmitter(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"gigabit-transmitter",sName)
		self.SetTip("")
		self.AddParamater("destination", "string", "10.10.0.1:1234", "the destination")
		self.AddParamater("port", "int", "4000", "the source port")
		self.AddParamater("mtu", "int", "1450", "the size of each generated packet (including headers)")
		self.AddParamater("bitrate", "int", "100", "in megabits per seconds, bitrate of the generated stream")
		self.AddParamater("fps", "int", "25", "number of frames per second")
g_dComponents["gigabit-transmitter"] = gigabit_transmitter

class live_reader(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"live-reader",sName)
		self.SetTip("This component finds a component a runtime and creates a route from that component to this. It also buffers the received packets untill the component is scheduled. Using this technique a live captured stream can be tapped into and routed to this component.")
		self.AddParamater("url", "string", "", "if defined, create a route from the component out which uses this url")
g_dComponents["live-reader"] = live_reader

class restamp(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"restamp",sName)
		self.SetTip("This components buffers as many packets as the reordering delay and release them we correctly generated DTSs inferred from PTS only streams. This component is only useful when the DTS is corrupted or undefined. CAVEAT: Two packets with the same PTS will cause a RuntuimeError.")
		self.AddParamater("delay", "int", "2", "the maximal reordering delay in packets")
g_dComponents["restamp"] = restamp

class statistics(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"statistics",sName)
		self.SetTip("This component generates at regular intervals information about the passing stream")
		self.AddParamater("interval", "int", "5000", "in ms, the amount of time between two reports, -1 disables the reports")
		self.AddParamater("log", "string", "", "if defined, the path where to log information about the passing packets")
		self.AddParamater("label", "string", "", "if defined, label used for each entry in the log, this is needed when sending multiple statistics output to one single file")
		self.AddParamater("append", "bool", "true", "if true, append data to the log, if false, overwrite the log")
		self.AddParamater("overhead", "int", "0", "the amount of header overhead from network headers to add to the packet size")
		self.AddParamater("draw", "bool", "false", "if true, when an end-packet is received, draw a graph of the bandwidth, requires that \textit{log} is defined")
g_dComponents["statistics"] = statistics

class svc_analyzer(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"svc-analyzer",sName)
		self.SetTip("This component constructs an inter layer bandwidth comparison at the end of a passing svc stream. A 3D graph is constructed with for each layer a cube scaled with the relative weight of the layer in the total bandwidth. The graph is generated as a maple worksheet (\url{www.maplesoft.com}).")
		self.AddParamater("filename", "string", "", "the path of the maple worksheet with extension .mws")
		self.AddParamater("dimension", "int", "1", "in how many dimensions to scale the cube (1, 2 or 3)")
g_dComponents["svc-analyzer"] = svc_analyzer

class time_splitter(miscellaneous):
	def __init__(self,sName):
		miscellaneous.__init__(self,"time-splitter",sName)
		self.SetTip("Cyclically classifies each time interval by increasing the original xroute, so that xroute cycles through [route,route+split[")
		self.AddParamater("split", "int", "1", "in how many parts to split the stream")
		self.AddParamater("interval", "int", "10000", "in ms, the duration of one part")
		self.AddParamater("key", "bool", "false", "if true, split only if the frame is a keyframe, if false, omit this condition")
g_dComponents["time-splitter"] = time_splitter

class transformer(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("These components transform the received frames for example by transcoding or changing the header format")
g_dBaseComponents["transformer"] = transformer

class ffmpeg_decoder(transformer):
	def __init__(self,sName):
		transformer.__init__(self,"ffmpeg-decoder",sName)
		self.SetTip("This components decodes a video sequence and generates a stream of YUV packets")
		self.AddParamater("reset-on-reset", "bool", "true", "if true, reset the decoder when a reset-packet is received")
		self.AddParamater("frame-copy", "bool", "false", "if true, use frame copy as basic error concealment")
g_dComponents["ffmpeg-decoder"] = ffmpeg_decoder

class frame_transformer(transformer):
	def __init__(self,sName):
		transformer.__init__(self,"frame-transformer",sName)
		self.SetTip("Handles the mess caused by the MP4 container that strips and merges MP4A and H.264 frames, while TSs and RTP keep the frames in the original format. Reconstructs the header based on the meta data.")
		self.AddParamater("ES", "bool", "true", "if true, convert the packets to ES format, if false, convert to MP4 format")
g_dComponents["frame-transformer"] = frame_transformer

class transcoder_audio(transformer):
	def __init__(self,sName):
		transformer.__init__(self,"transcoder-audio",sName)
		self.SetTip("Decodes received packets and reencodes them using the specified settings. This component runs the transcoding in a seperate thread and may consume all CPU.")
		self.AddParamater("output-codec", "string", "mp4a", "the target codec")
		self.AddParamater("bitrate", "int", "-1", "the target bitrate, -1 implies maintaining the same bitrate")
		self.AddParamater("target", "string", "", "if defined, use specific encoding settings for this target, values: iphone, ipad, youtube")
		self.AddParamater("route", "int", "200", "xroute assigned to the transcoded audio frames")
g_dComponents["transcoder-audio"] = transcoder_audio

class transcoder_video(transformer):
	def __init__(self,sName):
		transformer.__init__(self,"transcoder-video",sName)
		self.SetTip("Decodes received packets and reencodes them using the specified settings. This component runs the transcoding in a seperate thread and may consume all CPU. The component works best effort and can not garantee realtime transcoding.")
		self.AddParamater("output-codec", "string", "h264", "the target codec")
		self.AddParamater("bitrate", "int", "-1", "the target bitrate, -1 implies maintaining the same bitrate")
		self.AddParamater("width", "int", "0", "the target width, 0 implies maintaining the same width")
		self.AddParamater("height", "int", "0", "the target height, 0 implies maintaining the same height")
		self.AddParamater("framerate", "int", "-1", "the new frame rate which must be less or equal to the current frame rate, -1 implies maintaning the same frame rate")
		self.AddParamater("mov-frame", "bool", "false", "if true, generate H.264 in MOV/MP4 frames")
		self.AddParamater("target", "string", "", "if defined, use specific encoding settings for this target, values: iphone, ipad, youtube")
g_dComponents["transcoder-video"] = transcoder_video

class private(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Collection of private components, not for distribution")
g_dBaseComponents["private"] = private

class AQUA_monitor(private):
	def __init__(self,sName):
		private.__init__(self,"AQUA-monitor",sName)
		self.SetTip("Monitor for Aqua")
		self.AddParamater("media-file", "string", "", "the yuv-avi with the decoded video")
		self.AddParamater("watermark-file", "string", "", "the watermark extracted by the WM")
		self.AddParamater("hash-file", "string", "", "the hash extracted by the PH")
		self.AddParamater("ber-file", "string", "", "the comparison between the WM and the PH")
		self.AddParamater("loss", "double", "0.0", "the loss chance to determine the scenario in the log")
		self.AddParamater("strength", "double", "10.0", "the strength of the watermark")
		self.AddParamater("fragmented", "bool", "true", "if true, call the scripts after each reset")
		self.AddParamater("watermark", "bool", "true", "if true, consider watermarking")
g_dComponents["AQUA-monitor"] = AQUA_monitor

class AQUA_server(private):
	def __init__(self,sName):
		private.__init__(self,"AQUA-server",sName)
		self.SetTip("")
		self.AddParamater("port", "int", "8080", "the port on which to listen for incoming HTTP requests")
		self.AddParamater("strength", "double", "12", "the strength of the watermark")
g_dComponents["AQUA-server"] = AQUA_server

class AQUA_writer(private):
	def __init__(self,sName):
		private.__init__(self,"AQUA-writer",sName)
		self.SetTip("Writes the content of each received packet to a file and deletes the packet. When an end-packet is received, the file is closed.")
		self.AddParamater("output", "string", "", "the name of the yuv file")
		self.AddParamater("output-transform", "string", "", "the name of the avi file")
		self.AddParamater("flush", "bool", "false", "if true, flush the IO buffer after each write")
		self.AddParamater("fragmented", "bool", "false", "if true, after each reset, close the container and open a new container with a name of the form e.g. demo-0.avi, demo-1.avi, demo-2.avi, etc.")
g_dComponents["AQUA-writer"] = AQUA_writer

class system(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
g_dBaseComponents["system"] = system

class block(system):
	def __init__(self,sName):
		system.__init__(self,"block",sName)
		self.SetTip("This component takes Sirannon configuration and loads this configuration within itself. It allows grouping of components into one component that can be used in many configurations. Packets are sent to or received from the surrounding scope of the block using the \textit{in} component for input and \textit{out} component for output.")
		self.AddParamater("config", "string", "", "path of the sirannon configuration file")
		self.AddParamater("param1", "string", "", "if defined, command line parameter 1 for the configuration file")
		self.AddParamater("param2", "string", "", "if defined, command line parameter 2 for the configuration file")
		self.AddParamater("param3", "string", "", "if defined, command line parameter 3 for the configuration file")
		self.AddParamater("param4", "string", "", "if defined, command line parameter 4 for the configuration file")
		self.AddParamater("param5", "string", "", "if defined, command line parameter 5 for the configuration file")
		self.AddParamater("param6", "string", "", "if defined, command line parameter 6 for the configuration file")
		self.AddParamater("param7", "string", "", "if defined, command line parameter 7 for the configuration file")
		self.AddParamater("param8", "string", "", "if defined, command line parameter 8 for the configuration file")
		self.AddParamater("param9", "string", "", "if defined, command line parameter 9 for the configuration file")
		self.AddParamater("param10", "string", "", "if defined, command line parameter 10 for the configuration file")
		self.AddParamater("param11", "string", "", "if defined, command line parameter 11 for the configuration file")
		self.AddParamater("param12", "string", "", "if defined, command line parameter 12 for the configuration file")
		self.AddParamater("param13", "string", "", "if defined, command line parameter 13 for the configuration file")
		self.AddParamater("param14", "string", "", "if defined, command line parameter 14 for the configuration file")
		self.AddParamater("param15", "string", "", "if defined, command line parameter 15 for the configuration file")
g_dComponents["block"] = block

class discard(system):
	def __init__(self,sName):
		system.__init__(self,"discard",sName)
		self.SetTip("Any received packet will be deleted.")
g_dComponents["discard"] = discard

class dummy(system):
	def __init__(self,sName):
		system.__init__(self,"dummy",sName)
		self.SetTip("This component does absolutely nothing!")
g_dComponents["dummy"] = dummy

class _in(system):
	def __init__(self,sName):
		system.__init__(self,"in",sName)
		self.SetTip("This component finds a component a runtime and creates a route from that component to this. Using this technique a live captured stream can be tapped into and routed to this component.")
		self.AddParamater("url", "string", "", "if defined, create a route from the component 'out' which uses this url")
g_dComponents["in"] = _in

class out(system):
	def __init__(self,sName):
		system.__init__(self,"out",sName)
		self.SetTip("This component declares a url to be associated with this component and routes to components subscribing to this url")
		self.AddParamater("url", "string", "", "if defined, links this component with this url, when a new component 'in' is made")
g_dComponents["out"] = out

class sink(system):
	def __init__(self,sName):
		system.__init__(self,"sink",sName)
		self.SetTip("When this component receives an end-packet the program will terminate gracefully. In case of both an audio and a video stream, an end-packet for both audio and video needs to be received (if set).")
		self.AddParamater("video-mode", "bool", "true", "if true, a video end-packet must be received")
		self.AddParamater("audio-mode", "bool", "false", "if true, an audio end-packet must be received")
		self.AddParamater("count", "int", "0", "if larger than 0, the number of end-packets which must be received")
g_dComponents["sink"] = sink

class time_out(system):
	def __init__(self,sName):
		system.__init__(self,"time-out",sName)
		self.SetTip("If this component does not receive any packet within a given interval after the last packet, forcibly terminate the program or generate an end-packet.")
		self.AddParamater("time-out", "int", "1000", "in ms, the maximum interval in which no packets are received")
		self.AddParamater("kill", "bool", "true", "if true, terminate the program, if false, generate an end-packet")
g_dComponents["time-out"] = time_out

class media_server(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Media Servers are complex components, listening to a specific TCP port for incoming connections and dynamically creating a session as nested component for each new connection. URLs in the request should be of the form: \texttt{<protocol>://<server-address>/<application>/<media>}. For example \texttt{rtmpt://myserver.com/FILE/flash/example.flv} with server(\texttt{myserver.com}), application(\texttt{FILE}), media(\texttt{flash/example.flv}). Each session creates a nested component of the type \textit{application} given in the URL. Typical applications are FILE, HTTP, RTMP-proxy, RTMPT-proxy, RTSP-proxy, HTTP-proxy.")
		self.AddParamater("interface", "string", "", "if defined, listen to incoming connections on this specific interface")
		self.AddParamater("media", "string", "dat/media", "path of the folder in which to search for requested files")
g_dBaseComponents["media-server"] = media_server

class DIOR_server(media_server):
	def __init__(self,sName):
		media_server.__init__(self,"DIOR-server",sName)
		self.SetTip("Listens for incoming HTTP connections. TO BE FINISHED")
		self.AddParamater("port", "int", "80", "listen to this port for incoming connections")
g_dComponents["DIOR-server"] = DIOR_server

class HTTP_server(media_server):
	def __init__(self,sName):
		media_server.__init__(self,"HTTP-server",sName)
		self.SetTip("Listens for incoming HTTP connections. Does not support pausing or seeking. In the URL use application HTTP for file based transfer of requested file. Use the application FILE@FORMAT (eg. FILE@FLV, FILE@WEBM) to change the container type. Example URLs: \texttt{http://myserver.com/HTTP/demo.mov}, \texttt{http://192.168.1.3/FILE@FLV/demo.mov}")
		self.AddParamater("port", "int", "80", "listen to this port for incoming connections")
		self.AddParamater("cache", "bool", "false", "if true, cache generated HLS segments")
		self.AddParamater("cached", "bool", "false", "if true, used cached segments and playlists when available")
		self.AddParamater("segment", "int", "10", "in seconds, the duration of one HLS segment")
		self.AddParamater("high", "int", "1000", "in kbit/s, the bitrate of the high quality HLS encoding")
		self.AddParamater("medium", "int", "500", "in kbit/s, the bit rate of the medium quality HLS encoding")
		self.AddParamater("low", "int", "200", "in kbits/s, the bit rate of low quality HLS encoding")
g_dComponents["HTTP-server"] = HTTP_server

class RTMP_server(media_server):
	def __init__(self,sName):
		media_server.__init__(self,"RTMP-server",sName)
		self.SetTip("Listens for incoming RTMP connections. Supports pausing and seeking. Performs a cryptographical handshake to enable Flash Player to play H.264/AVC and MPEG4-Audio.")
		self.AddParamater("port", "int", "1935", "listen to this port for incoming connections")
g_dComponents["RTMP-server"] = RTMP_server

class RTMPT_server(media_server):
	def __init__(self,sName):
		media_server.__init__(self,"RTMPT-server",sName)
		self.SetTip("Listens for incoming RTMPT connections. This is the tunneled version of RTMP.")
		self.AddParamater("port", "int", "80", "listen to this port for incoming connections")
g_dComponents["RTMPT-server"] = RTMPT_server

class RTSP_server(media_server):
	def __init__(self,sName):
		media_server.__init__(self,"RTSP-server",sName)
		self.SetTip("Listens for RTSP connections. Supports pausing, but not seeking.")
		self.AddParamater("port", "int", "554", "listen to this port for incoming connections")
g_dComponents["RTSP-server"] = RTSP_server

class transmitter(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Transmitters provide the interface to the network for the following protocols: UDP, TCP and RTP/UDP. These components send the packets without any delay or buffering.")
		self.AddParamater("port", "int", "4000", "source port")
		self.AddParamater("destination", "string", "127.0.0.1:5000", "the IP address of the receiver")
		self.AddParamater("buffer", "int", "0", "if >0, the size of the protocol buffer, your OS must still accept this value, check 'UDP Buffer Sizing' in Google for more information")
		self.AddParamater("extension", "bool", "false", "if true, add an additional header with sirannon frame numbers to the packet, although making it incompatible with a standard player (except for RTP)")
		self.AddParamater("multicast-TTL", "int", "-1", "the TTL when sending to a multicast destination (not for TCP), -1 disables this")
g_dBaseComponents["transmitter"] = transmitter

class RTP_transmitter(transmitter):
	def __init__(self,sName):
		transmitter.__init__(self,"RTP-transmitter",sName)
		self.SetTip("Provides the RTP/UDP protocol using the open source library jrtplib RTCP packets are automatically generated. The extra information of the sirannon (sirannon-extension) is added as a header extension in RTP packets and this keeps it compatible with a standard player.")
		self.AddParamater("pts", "bool", "false", "if true, the streamer uses the PTS of a packet instead of the DTS as time stamp in the RTP header. This can solve the problem where VLC ometimes interprets the RTP timestamp as a PTS instead of a DTS")
		self.AddParamater("payload", "int", "-1", "payload type (PT) of the RTP packets, -1 means leaving the decision to the component")
		self.AddParamater("tracefile", "string", "", "if defined, the path of the trace where to log information about the sent packets")
		self.AddParamater("mtu", "int", "1500", "in bytes, the maximum packet size accepted by the RTP session")
		self.AddParamater("force-sequence-number", "bool", "false", "if true, force the RTP sequence number to follow the unitnumber, hence if you remove packets beforehand, the sequence number will also have gaps, if false, use the default RTP implementation")
g_dComponents["RTP-transmitter"] = RTP_transmitter

class TCP_transmitter(transmitter):
	def __init__(self,sName):
		transmitter.__init__(self,"TCP-transmitter",sName)
		self.SetTip("Provides a non-blocking TCP socket to the network.")
		self.AddParamater("connect", "bool", "true", "if true, connect to the server, if false, listen for an incoming connection")
g_dComponents["TCP-transmitter"] = TCP_transmitter

class UDP_transmitter(transmitter):
	def __init__(self,sName):
		transmitter.__init__(self,"UDP-transmitter",sName)
		self.SetTip("Provides a non-blocking UDP socket to the network.")
g_dComponents["UDP-transmitter"] = UDP_transmitter

class scheduler(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Schedulers introduce real-time behavior to the stream (readers generate packets at an arbitrary speed). It also introduces correct real-time behavior to streams coming from several receivers. It buffers all incoming packets and releases them at the rate set by the packet's decoding time stamp (DTS). Some schedulers introduce shifts to these time stamps order to obtain for example a smoother bandwidth usage.")
		self.AddParamater("delay", "int", "100", "in ms, the scheduler starts working delay ms after the first packet")
		self.AddParamater("buffer", "int", "1000", "in ms, the size in time of the buffer")
		self.AddParamater("absolute-delay", "int", "0", "in ms, start scheduling absolule-delay ms after the start of Sirannon, 0 disables this")
		self.AddParamater("pause", "bool", "false", "if true, pause scheduling until the function play is called")
		self.AddParamater("speed", "double", "1.0", "the factor at which to schedule slower or faster than real-time")
		self.AddParamater("precise", "bool", "false", "if true, run the scheduler in a seperate thread with a quantum of 1 ms for precise timings")
g_dBaseComponents["scheduler"] = scheduler

class basic_scheduler(scheduler):
	def __init__(self,sName):
		scheduler.__init__(self,"basic-scheduler",sName)
		self.SetTip("Simplest form in which the packets are sent at the time solely indicated by the DTS. This leads to a burst of packets for each frame since all the packets have the same DTS.")
g_dComponents["basic-scheduler"] = basic_scheduler

class frame_scheduler(scheduler):
	def __init__(self,sName):
		scheduler.__init__(self,"frame-scheduler",sName)
		self.SetTip("The packets belonging to a frame are smoothed in time over the duration of the frame (instead of sending an entire frame in one burst). Caveat, make sure the delay is longer than the duration of one frame!")
g_dComponents["frame-scheduler"] = frame_scheduler

class gop_scheduler(scheduler):
	def __init__(self,sName):
		scheduler.__init__(self,"gop-scheduler",sName)
		self.SetTip("The packets belonging to a GOP are smoothed in time over the entire duration of that GOP. Caveat, make sure the delay is longer than the duration of one GOP!")
g_dComponents["gop-scheduler"] = gop_scheduler

class qmatch_scheduler(scheduler):
	def __init__(self,sName):
		scheduler.__init__(self,"qmatch-scheduler",sName)
		self.SetTip("")
		self.AddParamater("interleave1", "bool", "false", "???")
		self.AddParamater("interleave2", "bool", "false", "???")
		self.AddParamater("interleave3", "bool", "false", "???")
g_dComponents["qmatch-scheduler"] = qmatch_scheduler

class svc_scheduler(scheduler):
	def __init__(self,sName):
		scheduler.__init__(self,"svc-scheduler",sName)
		self.SetTip("The packets belonging to one temporal pyramid are smoothed in time over the entire duration of that pyramid. Caveat, make sure the delay is longer than the duration of one pyramid!")
g_dComponents["svc-scheduler"] = svc_scheduler

class window_scheduler(scheduler):
	def __init__(self,sName):
		scheduler.__init__(self,"window-scheduler",sName)
		self.SetTip("The packets are smoothed in time over a fixed non-sliding window. Caveat, make sure the delay is longer than the duration of one window!")
		self.AddParamater("window", "int", "900", "in ms, the size of the fixed window")
g_dComponents["window-scheduler"] = window_scheduler

class reader(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Readers form the primary source of data in the sirannon. When the file reaches its end, a reader either closes the file, generating an end-packet or loops creating a reset-packet. A reader checks if no buffers downstream (typically a scheduler buffer) are full before processing the next frame.")
		self.AddParamater("filename", "string", "", "the path of the file to open")
		self.AddParamater("loop", "int", "1", "the number of times to play the video, -1 being infinite, 0 interpreted as 1")
		self.AddParamater("video-route", "int", "100", "the xroute that will be assigned to packets containing video")
		self.AddParamater("audio-route", "int", "200", "the xroute that will be assigned to packets containing audio")
		self.AddParamater("dts-start", "int", "1000", "in ms, the timestamp that will be added to the DTS & PTS of each frame")
g_dBaseComponents["reader"] = reader

class AMR_WBP_reader(reader):
	def __init__(self,sName):
		reader.__init__(self,"AMR-WBP-reader",sName)
		self.SetTip("Reads in an AMR-WB+ encoded sequence in file storage format. The component releases one superframe per packet.")
g_dComponents["AMR-WBP-reader"] = AMR_WBP_reader

class avc_reader(reader):
	def __init__(self,sName):
		reader.__init__(self,"avc-reader",sName)
		self.SetTip("Reads in a raw H264 video file. Each generated \textit{MediaPacket} contains one H.264 NAL-unit, possibly generating multiple \textit{MediaPackets} per frame.")
		self.AddParamater("skip-SEI", "bool", "false", "if true, ignore the SEI NAL-units")
		self.AddParamater("skip-AUD", "bool", "false", "if true, ignore the AUD NAL-units")
		self.AddParamater("mov-frame", "bool", "false", "if true, convert NAL-units into MP4/MOV/F4V frames")
g_dComponents["avc-reader"] = avc_reader

class basic_reader(reader):
	def __init__(self,sName):
		reader.__init__(self,"basic-reader",sName)
		self.SetTip("Reads in any container in large chunks. Its main use is the HTTP transmission in chunks of a container.")
		self.AddParamater("chunk-size", "int", "64000", "in bytes, the maximum size of a chunk")
		self.AddParamater("length", "int", "0", "in ms, if > 0, provides the reader with the duration of the file so it can guess the DTS")
g_dComponents["basic-reader"] = basic_reader

class ffmpeg_reader(reader):
	def __init__(self,sName):
		reader.__init__(self,"ffmpeg-reader",sName)
		self.SetTip("Reads in a wide variety of containers supported by ffmpeg. Audio and video are put into different MediaPackets. Per cycle, the reader processes one video frame (if present) and associated audio, possibly generating multiple packets. A separate end- or reset-packet is generated for audio & video. Note, ffmpeg-reader can also process audio only files.")
		self.AddParamater("video-mode", "bool", "true", "if true, video will be read from the container, if false video will be ignored")
		self.AddParamater("audio-mode", "bool", "true", "if true, audio will be read from the container, if false audio will be ignored")
		self.AddParamater("min-ts", "int", "-1", "in ms, the timestamp to jump to into the stream, -1 implying no seek")
		self.AddParamater("max-ts", "int", "-1", "in ms, the maximum timestamp to read from the file, -1 implying no limit")
		self.AddParamater("dts-start", "int", "1000", "in ms, specifies the value of the timestamp of the first frame")
		self.AddParamater("add-parameter-sets", "bool", "true", "H.264/AVC only, if true, extract the parameter sets from the container and insert them into the stream")
		self.AddParamater("repeat-parameter-sets", "bool", "false", "H.264/AVC only, if true, repeat the parameter sets before each IDR frame")
		self.AddParamater("mov-frame", "bool", "false", "MOV/MP4/F4V container only, if true, keep frames in the format of the container, as opposed to annex-B H.264/AVC streams with start codes before each NAL unit")
		self.AddParamater("skip-SEI", "bool", "false", "if true, remove SEI NALUs from the stream")
		self.AddParamater("skip-AUD", "bool", "false", "if true, remove AUD NALUs from the stream")
		self.AddParamater("fix-PTS", "bool", "false", "if true, parse H.264 frames to extract the POC from which to calculate the correct PTS")
g_dComponents["ffmpeg-reader"] = ffmpeg_reader

class unpacketizer(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
g_dBaseComponents["unpacketizer"] = unpacketizer

class AMR_unpacketizer(unpacketizer):
	def __init__(self,sName):
		unpacketizer.__init__(self,"AMR-unpacketizer",sName)
		self.SetTip("Unpacketizes an AMR-NB/-WB stream packed according to RFC 3267.")
g_dComponents["AMR-unpacketizer"] = AMR_unpacketizer

class AVC_unpacketizer(unpacketizer):
	def __init__(self,sName):
		unpacketizer.__init__(self,"AVC-unpacketizer",sName)
		self.SetTip("The fragmented or aggregated frames are transformed again into their original NAL units, as defined in RFC 3984. It performs the reverse operation of the AVC-packetizer.")
		self.AddParamater("startcodes", "bool", "true", "if true, add startcodes to the unpacked NALs")
		self.AddParamater("strict-annex-b", "bool", "true", "if false, all startcodes will be 4 bytes")
g_dComponents["AVC-unpacketizer"] = AVC_unpacketizer

class MP2_unpacketizer(unpacketizer):
	def __init__(self,sName):
		unpacketizer.__init__(self,"MP2-unpacketizer",sName)
		self.SetTip("Unpacketizes a stream fragmented with MP2 packetization for RTP (RFC 2250) by for example (MP2-packetizer).")
g_dComponents["MP2-unpacketizer"] = MP2_unpacketizer

class MP4_unpacketizer(unpacketizer):
	def __init__(self,sName):
		unpacketizer.__init__(self,"MP4-unpacketizer",sName)
		self.SetTip("Unpacketizes a stream fragmented with MP4 packetization for RTP (RFC 3640) by for example MP4-packetizer.")
g_dComponents["MP4-unpacketizer"] = MP4_unpacketizer

class PES_unpacketizer(unpacketizer):
	def __init__(self,sName):
		unpacketizer.__init__(self,"PES-unpacketizer",sName)
		self.SetTip("One or more PES-packets belonging to one frame are split again into the original parts. It performs the reverse operation of the PES-packetizer.")
g_dComponents["PES-unpacketizer"] = PES_unpacketizer

class sirannon_unpacketizer(unpacketizer):
	def __init__(self,sName):
		unpacketizer.__init__(self,"sirannon-unpacketizer",sName)
		self.SetTip("Unpacketizes a stream fragmented by the internal packetizer of Sirannon (sirannon-packetizer).")
		self.AddParamater("recover-frame", "bool", "false", "if true, unpacked parts of a damage frame instead of discarding the entire frame")
		self.AddParamater("error-on-loss", "bool", "false", "if true, throw an exception when packet loss occurs")
g_dComponents["sirannon-unpacketizer"] = sirannon_unpacketizer

class high_level(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("")
g_dBaseComponents["high-level"] = high_level

class TS_segmenter(high_level):
	def __init__(self,sName):
		high_level.__init__(self,"TS-segmenter",sName)
		self.SetTip("This component ingest a video container and generates a series of transport streams at varies bit rates")
		self.AddParamater("media", "string", "dat/media", "path of the folder in which to search for requested files")
		self.AddParamater("filename", "string", "", "the path of the container to stream (relative to the folder specified by media)")
		self.AddParamater("target", "string", "", "string indicating the target display (iphone, ipad, youtube)")
		self.AddParamater("duration", "int", "10", "in seconds the maximum duration of one segment")
		self.AddParamater("bitrate-0", "int", "0", "in kbps, the 1st bit rate set point, if 0, do not transcode and only segment and do not check for more bitrates")
		self.AddParamater("width-0", "int", "0", "the 1st width set point")
		self.AddParamater("height-0", "int", "0", "the 1st height set point")
		self.AddParamater("bitrate-1", "int", "0", "in kbps, the 2nd bit rate set point, if 0, do not transcode and only segment and do not check for more bitrates")
		self.AddParamater("width-1", "int", "0", "the 2nd width set point")
		self.AddParamater("height-1", "int", "0", "the 2nd height set point")
		self.AddParamater("bitrate-2", "int", "0", "in kbps, the 3rd bit rate set point, if 0, do not transcode and only segment and do not check for more bitrates")
		self.AddParamater("width-2", "int", "0", "the 3rd width set point")
		self.AddParamater("height-2", "int", "0", "the 3rd height set point")
g_dComponents["TS-segmenter"] = TS_segmenter

class streamer(high_level):
	def __init__(self,sName):
		high_level.__init__(self,"streamer",sName)
		self.SetTip("This component provides a streaming solution without having to construct a scheme of components. Its many parameters reflect the options of the hidden underlying components (delay, port, destination\textellipsis).")
		self.AddParamater("filename", "string", "", "the path of the container to stream")
		self.AddParamater("mode", "string", "default", "what content to read from the container (video, audio, default)")
		self.AddParamater("loop", "int", "1", "the number of times to play the stream, -1 being infinite, 0 interpreted as 1")
		self.AddParamater("seek", "int", "0", "the time index to seek to into the container")
		self.AddParamater("aggregate", "bool", "false", "option of some packetizers")
		self.AddParamater("multiplexer-delay", "int", "0", "delay of the multiplexer, used for transport streams or RTMP")
		self.AddParamater("scheduler", "string", "basic", "which sort of scheduler to use (basic, frame, gop, window)")
		self.AddParamater("scheduler-buffer", "int", "1000", "in ms, the buffer size in time of the scheduler")
		self.AddParamater("scheduler-delay", "int", "0", "in ms, the delay of the scheduler")
		self.AddParamater("loss", "double", "0.0", "if larger than 0, randomly lose packets with the give chance")
		self.AddParamater("transmitter", "string", "rtp", "type of the transmitter(rtp, udp, tcp)")
		self.AddParamater("port", "int", "4000", "source port of the transmitter")
		self.AddParamater("destination", "string", "127.0.0.1:5000", "destination of the transmitter")
		self.AddParamater("ts-mode", "bool", "false", "if true, multiplex the streams into a transport stream")
		self.AddParamater("RTMP-mode", "bool", "false", "if true, muliplex the streams into RTMP chunk streams as output from this component")
		self.AddParamater("RTMP-streamID", "int", "1", "streamID used for each RTMP chunk stream")
		self.AddParamater("RTMP-video-chunkID", "int", "7", "RTMP chunk stream ID for video")
		self.AddParamater("RTMP-audio-chunkID", "int", "6", "RTMP chunk stream ID for audio")
		self.AddParamater("RTMP-chunk-size", "int", "4096", "chunk size for the RTMP chunk streams")
g_dComponents["streamer"] = streamer

class classifier(core):
	def __init__(self,sType,sName):
		core.__init__(self,sType,sName)
		self.SetTip("Classifiers add an offset to the xroute of a packet if it meets a certain condition. See section \ref{sec:diff} for an example. Several classifiers can be chained to obtain a more precision classification.")
		self.AddParamater("discard", "bool", "false", "if true, delete the packet instead if the condition is met")
		self.AddParamater("sender-trace", "string", "", "if defined, the path where to log information about the packets entering the classifier")
		self.AddParamater("receiver-trace", "string", "", "if defined, the path where to log information about the packets exiting the classifier, implies discard is true")
g_dBaseComponents["classifier"] = classifier

class avc_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"avc-classifier",sName)
		self.SetTip("This components classifies the many different types of NAL units present in H264, far the beyond the common I, B and P frames.")
		self.AddParamater("I", "int", "0", "the offset if the media-packet is an I slice")
		self.AddParamater("SI", "int", "0", "the offset if the media-packet is an SI slice")
		self.AddParamater("EI", "int", "0", "the offset if the media-packet is an EI slice")
		self.AddParamater("I(B)", "int", "0", "the offset if the media-packet is an I (data partition B) slice")
		self.AddParamater("I(C)", "int", "0", "the offset if the media-packet is an I (data partition C) slice")
		self.AddParamater("P", "int", "0", "the offset if the media-packet is a P slice")
		self.AddParamater("SP", "int", "0", "the offset if the media-packet is an SP slice")
		self.AddParamater("EP", "int", "0", "the offset if the media-packet is an EP slice")
		self.AddParamater("P(B)", "int", "0", "the offset if the media-packet is a P (data partition B) slice")
		self.AddParamater("P(C)", "int", "0", "the offset if the media-packet is a P (data partition C) slice")
		self.AddParamater("B", "int", "0", "the offset if the media-packet is a B slice")
		self.AddParamater("EB", "int", "0", "the offset if the media-packet is an EB slice")
		self.AddParamater("B(B)", "int", "0", "the offset if the media-packet is a B (data partition B) slice")
		self.AddParamater("B(C)", "int", "0", "the offset if the media-packet is a B (data partition C) slice")
		self.AddParamater("E", "int", "0", "the offset if the media-packet is a prefix NAL")
		self.AddParamater("PPS", "int", "0", "the offset if the media-packet is a PPS unit")
		self.AddParamater("SPS", "int", "0", "the offset if the media-packet is an SPS unit")
		self.AddParamater("ESPS", "int", "0", "the offset if the media-packet is an extended SPS unit")
		self.AddParamater("SEI", "int", "0", "the offset if the media-packet is a SEI unit")
		self.AddParamater("default", "int", "0", "the offset if the packet does not belong to any of the above")
g_dComponents["avc-classifier"] = avc_classifier

class count_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"count-classifier",sName)
		self.SetTip("This component classifies or discards every nth frame if 'n % cycle' is non zero. By setting discard to true, this component will essentially discard all but every nth frame.")
		self.AddParamater("cycle", "int", "10", "do not classify every 'cycle * n'th frame, e.g. if cycle is 10, do not classify frames 0, 10, 20, 30, etc.")
g_dComponents["count-classifier"] = count_classifier

class fixed_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"fixed-classifier",sName)
		self.SetTip("Classifies the packets, slice or frames based on fixed indices provided by the user. CAVEAT: Indices must rise strict monotonously; Non slice NAL units such as PPS, SPS, SEI, E, are not counted as slices while different SVC layers are also counted as slices. For example an MVC stream with 4 slices and with 2 layers per frame will be counted as 8 slices.")
		self.AddParamater("mode", "string", "frame", "whether the indices inputted in values or values-file specify packets, slices or frames, accepted values: frame, slice, packet")
		self.AddParamater("values", "string", "", "if defined, a comma seperated string of indices, indicating which packet, slice or frame to classify counted from 0")
		self.AddParamater("values-file", "string", "", "if defined, a file containing a new line seperated string of indices, indicating which packet, slice or frame to classify counted from 0")
		self.AddParamater("xroute", "int", "1", "offset added to the xroute of the packet if the packet is classified")
g_dComponents["fixed-classifier"] = fixed_classifier

class frame_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"frame-classifier",sName)
		self.SetTip("Classifies packets based on the type of frame they belong to. The parameter I stands for all sort of I frames (I, IDR, EI, SI...), the same holds for other frame types.")
		self.AddParamater("I", "int", "0", "the offset if the media-packet is an I slice or frame")
		self.AddParamater("B", "int", "0", "the offset if the media-packet is an B slice or frame")
		self.AddParamater("P", "int", "0", "the offset if the media-packet is an P slice or frame")
		self.AddParamater("default", "int", "0", "the offset if the frame type is none of the above")
g_dComponents["frame-classifier"] = frame_classifier

class gilbert_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"gilbert-classifier",sName)
		self.SetTip("This component has a random chance of classifying a packet using the Gilbert method.")
		self.AddParamater("alpha", "double", "0.01", "$\in[0,1]$, probability to transit from the GOOD to the BAD state")
		self.AddParamater("beta", "double", "0.1", "$\in[0,1]$, probability to transit from the BAD to the GOOD state")
		self.AddParamater("gamma", "double", "0.75", "$\in[0,1]$, probability to classify a packet in the BAD state")
		self.AddParamater("delta", "double", "0.01", "$\in[0,1]$, probability to classify a packet in the GOOD state")
		self.AddParamater("xroute", "int", "1", "offset if the condition is met")
g_dComponents["gilbert-classifier"] = gilbert_classifier

class nalu_drop_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"nalu-drop-classifier",sName)
		self.SetTip("Drops specific NAL units from an H.264/AVC encoded video stream. The indices of the NAL units to drop are provided in a tracefile. The first NAL unit of the stream has index 1.")
		self.AddParamater("drop-trace", "string", "", "path to file which contains information on which NAL units to drop. A sample drop-trace file is located in the src/Misc folder.")
		self.AddParamater("udpmode", "bool", "false", "set to true in case a UDP receiver is used instead of an RTP receiver. Sequence numbers are retained in UDP mode whereas in case of an RTP receiver and transmitter new sequence numbers are generated.")
g_dComponents["nalu-drop-classifier"] = nalu_drop_classifier

class random_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"random-classifier",sName)
		self.SetTip("This component has a random chance of classifying a packet using a uniform distribution.")
		self.AddParamater("P(loss)", "double", "0.01", "$\in[0,1]$, probability to classify the packet")
		self.AddParamater("xroute", "int", "0", "offset if the condition is met")
g_dComponents["random-classifier"] = random_classifier

class svc_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"svc-classifier",sName)
		self.SetTip("This component classifies an SVC stream. The partitioning is defined by a series of $(T,D,Q)$ triplets. For each passing NAL unit this series is reverse iterated. If the NAL unit depends on the $n^{th}$ triplet then it is classified as $layer-{n}$. When it didn't depend on any layer it is classified as $layer-{0}$. The triplets are entered in the form of $'T,D,Q:offset'$, for example $'4,0,0:2'$, meaning \textit{'add offset 2 to packets of temporal layer (T) 4 or higher'}.")
		self.AddParamater("layer", "int", "0", "offset for layer 0")
		self.AddParamater("layer-1", "string", "", "definintion and offset for layer 1")
		self.AddParamater("layer-2", "string", "", "definintion and offset for layer 2")
		self.AddParamater("layer-3", "string", "", "definintion and offset for layer 3")
		self.AddParamater("layer-4", "string", "", "definintion and offset for layer 4")
		self.AddParamater("layer-5", "string", "", "definintion and offset for layer 5")
		self.AddParamater("layer-6", "string", "", "definintion and offset for layer 6")
		self.AddParamater("layer-7", "string", "", "definintion and offset for layer 7")
g_dComponents["svc-classifier"] = svc_classifier

class time_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"time-classifier",sName)
		self.SetTip("Basic classifier based solely on the real time interval, relative to the first packet. The four time parameters combine to create an interval of the form: $t\in[start,stop[$, when step is defined, it must also hold that: $$t\in\bigcup-{n=1}^{+\infty}[n\cdot step,n\cdot step+delta[$$.")
		self.AddParamater("start", "int", "0", "in ms, the minimum of the range")
		self.AddParamater("stop", "int", "-1", "in ms, the maximum of the range, -1 being unlimited")
		self.AddParamater("step", "int", "-1", "in ms, the time between two steps, -1 disabling the step effect")
		self.AddParamater("delta", "int", "-1", "in ms, the duration of one step, -1 being unlimited")
		self.AddParamater("xroute", "int", "1", "offset if the condition is met")
g_dComponents["time-classifier"] = time_classifier

class timestamp_classifier(classifier):
	def __init__(self,sName):
		classifier.__init__(self,"timestamp-classifier",sName)
		self.SetTip("Basic classifier based solely on the decoding time stamp (DTS) interval, relative to the first packet.")
		self.AddParamater("start", "int", "0", "in DTS clock ($90kHz$), the minimum of the interval")
		self.AddParamater("stop", "int", "-1", "in DTS clock ($90kHz$), the maximum of the interval, -1 being unlimited")
		self.AddParamater("xroute", "int", "1", "offset if the condition is met")
g_dComponents["timestamp-classifier"] = timestamp_classifier

g_lComponents = g_dComponents.keys()
g_lComponents.sort()
g_lBaseComponents = ['classifier', 'core', 'demultiplexer', 'high-level', 'media-client', 'media-server', 'miscellaneous', 'multiplexer', 'packetizer', 'private', 'reader', 'receiver', 'scheduler', 'system', 'transformer', 'transmitter', 'unpacketizer', 'writer']
