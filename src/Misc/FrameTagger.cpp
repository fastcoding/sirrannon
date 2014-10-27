/*****************************************************************************
 * (c)2006-2010 Sirannon
 * Authors: Alexis Rombaut <alexis.rombaut@intec.ugent.be>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *****************************************************************************/
#include "FrameTagger.h"
#include "h264_avc.h"
#include "Frame.h"

/** FRAME ANALYZER
 * @component frame-analyzer
 * @type miscellaneous
 * @info This component parses elementary MPEG-1, MPEG-2, MPEG-4 or H.264/AVC streams
 * to determine the frame type.
 **/
REGISTER_CLASS( FrameTagger, "frame-analyzer" );

FrameTagger::FrameTagger( const string& name, ProcessorManager* pManager )
	: MediaProcessor(name, pManager)
{
}

void FrameTagger::receive( MediaPacketPtr& pckt )
{
	int iSize = pckt->size();
	uint8_t* pData = pckt->data();

	/* Check the codec of the packet */
	if( pckt->codec & codec_t::H264 )
	{
		if( pckt->mux == mux_t::ES )
		{
			/* Which NAL type? */
			int iNAL = H264_NAL_type( pData );

			/* Find the slice type if it is present */
			int iSlice = -1;
			if( iNAL == NAL_UNIT_TYPE_SLICE_DP_A or
				iNAL == NAL_UNIT_TYPE_SLICE_NON_IDR or
				iNAL == NAL_UNIT_TYPE_SLICE_SVC )
			{
				/* Skip the header */
				IBits oFrame( pData, iSize );
				oFrame.seek( H264_header_size( pData ) );

				/* Read the slice type */
				oFrame.read_uev();
				iSlice = oFrame.read_uev();
			}
			/* Now we can deduce the frame type */
			frame_t::type iFrame = H264FrameToXFrame( iNAL, iSlice );
			pckt->frame = iFrame;
		}
		else
		{
			if( pckt->key )
				pckt->frame = frame_t::IDR;
		}

	}
	else if( pckt->codec == codec_t::mp2v or pckt->codec == codec_t::mp1v )
	{
		/* Find a picture start code */
		int iPos = find_next_start_code( pData, iSize, 0x00000100 );
		if( iPos >= 0 and iPos + 6 <= iSize )
		{
			/* Start reading the frame after the picture startcode */
			IBits oFrame( pData + iPos + 4, 2 );

			/* Frame type is the second field */
			oFrame.read( 10 );
			int iFrame = oFrame.read( 3 );

			switch( iFrame )
			{
			case 1:
				pckt->frame = frame_t::I;
				break;

			case 2:
				pckt->frame = frame_t::P;
				break;

			case 3:
				pckt->frame = frame_t::B;
				break;
			}
		}
	}
	else if( pckt->codec == codec_t::mp4v )
	{
		/* Find a picture start code */
		int iPos = find_next_start_code( pData, iSize, 0x000001B6 );
		if( iPos >= 0 and iPos + 5 <= iSize )
		{
			IBits oFrame( pData + iPos + 4, 1 );
			int iFrame = oFrame.read( 2 );

			switch( iFrame )
			{
			case 0:
				pckt->frame = frame_t::I;
				break;

			case 1:
				pckt->frame = frame_t::P;
				break;

			case 2:
				pckt->frame = frame_t::B;
				break;
			}
		}
	}
	/* Done */
	debug( 1, "tagged: %s", pckt->c_str() );
	route( pckt );
}
