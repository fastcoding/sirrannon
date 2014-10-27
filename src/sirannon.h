#ifndef SIRANNON_H_
#define SIRANNON_H_
using namespace std;

#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define UNUSED __attribute__((unused))
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NOINLINE __attribute__((noinline))
#define MAY_ALIAS __attribute__((may_alias))
#else
#define UNUSED
#define ALWAYS_INLINE inline
#define NOINLINE
#define MAY_ALIAS
#endif

#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <string>

/* Constants */
const static uint32_t SIRANNON_MIN_QUANTUM = 1000;

/* Some predefined number to avoid mismatches (eg one zero too much) */
const static uint32_t KILO = 1000;
const static uint32_t MEGA = 1000000;
const static uint32_t GIGA = 1000000000;
const static uint32_t KIBI = 1024;
const static uint32_t MEBI = 1048576;
const static uint32_t GIBI = 1073741824;

/* Common classes */
class ContainerDescriptor;
class MediaProcessor;
class MediaPacket;
class MediaDescriptor;
class ProcessorManager;
class SirannonException;
class SirannonTime;

/* The basic header */
#include "ProcessorManager.h"

/* Namespaces */
using namespace std;

#endif /*SIRANNON_H_*/
