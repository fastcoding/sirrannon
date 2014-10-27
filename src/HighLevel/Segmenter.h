/*
 * Segmenter.h
 *
 *  Created on: Mar 29, 2011
 *      Author: arombaut
 */

#ifndef SEGMENTER_H_
#define SEGMENTER_H_
#include "MediaProcessor.h"
#include "Interfaces.h"

class Segmenter : public MediaProcessor, public NestedInterface
{
public:
	Segmenter( const string& sName, ProcessorManager* pScope );

	void handleError( SirannonException* pException, ProcessorManager* pManager, MediaProcessor* pProcessor );

protected:
	~Segmenter();

	void init( void );
	void process( void );
};

#endif /* SEGMENTER_H_ */
