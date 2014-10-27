/*
 * SirannonPrint.h
 *
 *  Created on: Nov 8, 2010
 *      Author: arombaut
 */

#ifndef SIRANNONPRINT_H_
#define SIRANNONPRINT_H_
#define SIRANNON_USE_BOOST_THREAD
#include "Boost.h"
#include "sirannon.h"

extern mutex oPrintMutex;

void SirannonPrint( int iLevel, const char* fmt, ... );

#endif /* SIRANNONPRINT_H_ */
