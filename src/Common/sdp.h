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
#ifndef SDP_H_
#define SDP_H_
#include "sirannon.h"

/* Construct the SDP from an existing description of a container */
int SDPConstruct( const ContainerDescriptor* pSrcContainer, int iPort, bool bMP2TS, ContainerDescriptor* pContainer );

/* Construct the SDP from opening the container with ffmpeg and parsing it */
int SDPConstruct( const string& sFile, int iPort, bool bMPEG2TS, ContainerDescriptor* pContainer, mux_t::type& oMux );

/* Generate the complete SDP string based on the parsed input container */
void SDPString( const ContainerDescriptor* oSDP, string& sSDP );

/* Parsing an existing SDP descriptor in a container descriptor */
int SDPParse( const string& sSDP, mux_t::type& oMux, ContainerDescriptor* pContainer );

#endif /*SDP_H_*/
