#ifdef WITH_LIBPCAP
#include "PcapWriter.h"

/**
 * @component PCAP-writer
 * @type miscellaneous
 * @properties unix
 * @param interface, string, eth0, the name of device on which to capture
 * @param filename, string, , the path of the file where to write the captured packets
 * @param filter, string, , if defined, overwrite the built-in filter
 * @param port, int, 1234, the UDP port on which to listen for traffic (when using the built-in filter)
 * @param bitrate, int, 20, in Mbps, estimated bitrate of stream which determines the underlying buffer size. When unsure of the bitrate, use a royal upper limit. If the debug reports dropped packets, increase this value.
 * @info Captures packets using a filter and saves those to a file. Uses libpcap for capturing. Needs to have permission to access the interfaces (eg. sudo).
 **/
REGISTER_CLASS( PcapWriter, "PCAP-writer" );

PcapWriter::PcapWriter( const string& sName, ProcessorManager* pScope )
	: MediaProcessor(sName, pScope), pHandle(NULL), pFile(NULL)
{
	mString["interface"] = "eth0";
	mBool["promisceous"] = false;
	mString["filter"] = "udp port %d";
	mString["filename"] = "";
	mInt["buffer"] = 64000;
}

PcapWriter::~PcapWriter()
{
	if( pHandle )
	{
		pcap_stat oError;
		pcap_stats( pHandle, &oError );
		print( 1, "closing pcap: received(%d) dropped_in_buffer(%d) dropped_by_interface(%d)", oError.ps_recv, oError.ps_drop, oError.ps_ifdrop );
		pcap_close( pHandle );
	}
}

void PcapWriter::init( void )
{
	/* Base class */
	MediaProcessor::init();

	/* Create a PCAP handle */
	const char* sDev = mString["interface"].c_str();
	pHandle = pcap_create( sDev, sError );
	if( not pHandle )
		PCAPError( this, "%s", sError );
	pcap_set_promisc( pHandle, int(mBool["promisceous"]) );
	pcap_set_snaplen( pHandle, 65535 );
	pcap_set_timeout( pHandle, 250 );
	pcap_set_buffer_size( pHandle, mInt["bitrate"] * 1000 * 1000 ); // Normally divide by 16
	pcap_activate( pHandle );

	/* Filter */
	char sExpr[1024];
	const char* sFilter = mString["filter"].c_str();
	if( not strlen(sFilter) )
		sFilter = "udp port %d";
	snprintf( sExpr, sizeof(sExpr), sFilter, mInt["port"] );
	struct bpf_program oFilter;
	bpf_u_int32 iMask, iNet;

	if( pcap_lookupnet( sDev, &iNet, &iMask, sError ) < 0 )
		PCAPError( this, "%s", sError );

	if( pcap_compile( pHandle, &oFilter, sExpr, 0, iNet ) < 0 )
		PCAPError( this, "%s", pcap_geterr(pHandle) );

	if( pcap_setfilter( pHandle, &oFilter ) < 0 )
		PCAPError( this, "%s", pcap_geterr(pHandle) );

	/* Output file */
	if( mString["filename"].length() )
	{
		pFile = pcap_dump_open( pHandle, mString["filename"].c_str() );
		if( not pFile )
			PCAPError( this, "%s", pcap_geterr(pHandle) );
	}
	createThread( bind( &PcapWriter::pcapLoop, this ) );
}

void PcapWriter::pcapLoop( void )
{
	while( not is_stopped() )
	{
		pcap_dispatch( pHandle, 0, pcap_dump, (u_char*)pFile );
		this_thread::interruption_point();
	}
}

#endif
