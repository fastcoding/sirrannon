#ifndef OSSUPPORT_H_
#define OSSUPPORT_H_

/* Printf modifiers */
#ifdef WIN32
#define LL "I64"
#else
#ifdef __LP64__
#define LL "l"
#else
#define LL "ll"
#endif
#endif

/* Joined functions */
int setNonBlocking( int fd );
char* strError( void );

#endif /* OSSUPPORT_H_ */
