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
#ifndef INTERFACES_H_
#define INTERFACES_H_
#include "sirannon.h"
#include "ProcessorManager.h"
#include "MediaDescriptor.h"

class ProcessorManager;
class NestedInterface
{
public:
	virtual ProcessorManager* getProcessorManager( void );
	virtual void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor ) = 0;

protected:
	NestedInterface( const string& sName, ProcessorManager* pScope )
	: oProcessorManager(sName, pScope, this){ };
	virtual ~NestedInterface( ) { };
	ProcessorManager oProcessorManager;
};

class SourceInterface
{
public:
	virtual int seek( uint32_t iTimeIndex ) = 0; /* This function may block */
	virtual bool ready( void ) const = 0;
	virtual const ContainerDescriptor* getDescriptor( void ) const;
	virtual void findSchedulers( void ) { };

protected:
	SourceInterface( ) { };
	virtual ~SourceInterface( ) { };

	MediaDescriptor* addMedia( void );
	ContainerDescriptor oContainer;
};

class BufferInterface
{
public:
	virtual int play( double fSpeed ) = 0; /* This function may block */
	virtual int pause( void ) = 0; /* This function may block */

protected:
	BufferInterface( ) { };
	virtual ~BufferInterface( ) { };
};

class TransmitterInterface
{
public:
	virtual uint32_t getSequenceNumber( void ) const = 0;
	virtual uint32_t getTimestamp( void ) const = 0;
	virtual uint32_t getPort( void ) const = 0;

protected:
	TransmitterInterface( ) { };
	virtual ~TransmitterInterface( ) { };
};

class StreamerInterface : public SourceInterface, public BufferInterface
{
public:
	virtual SourceInterface* getSource( void ) const { return pSource; }
	virtual BufferInterface* getBuffer( void ) const { return pBuffer; }
	virtual TransmitterInterface* getTransmitter( content_t::type iContent = content_t::video );

	virtual void createSource( void ) = 0;
	virtual void createBuffer( void ) = 0;
	virtual void createTransmitter( MediaDescriptor* pDesc = NULL ) = 0;
	virtual void createComponents( void );

protected:
	StreamerInterface( );
	virtual ~StreamerInterface( ) { };

	SourceInterface* pSource;
	BufferInterface* pBuffer;
	TransmitterInterface *pVideoTransmitter, *pAudioTransmitter;
};

class ClientInterface : public SourceInterface, public BufferInterface
{
public:
	ClientInterface() : bAutoPlay(true) { };

protected:
	bool bAutoPlay;
};

class RateController
{
public:
	virtual bool bufferFull( void ) const = 0;

protected:
	RateController( ) { };
	virtual ~RateController() { };
};

inline const ContainerDescriptor* SourceInterface::getDescriptor( void ) const
{
	return &oContainer;
}

inline MediaDescriptor* SourceInterface::addMedia( void )
{
	oContainer.push_back( MediaDescriptor() );
	oContainer.back().source = this;
	return &oContainer.back();
}

inline ProcessorManager* NestedInterface::getProcessorManager( void )
{
	return &oProcessorManager;
}

inline StreamerInterface::StreamerInterface( )
	: pSource(NULL), pBuffer(NULL), pVideoTransmitter(NULL), pAudioTransmitter(NULL)
{ };

inline TransmitterInterface* StreamerInterface::getTransmitter( content_t::type iContent )
{
	if( iContent == content_t::video )
		return pVideoTransmitter;
	else
		return pAudioTransmitter;
}

inline void StreamerInterface::createComponents( void )
{
	createSource();
	createBuffer();
	createTransmitter();
}

#endif /* INTERFACES_H_ */
