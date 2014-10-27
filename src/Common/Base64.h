#ifndef BASE64_H_
#define BASE64_H_

char* Base64Encode( const uint8_t* pSource, uint32_t iSize );
uint8_t* Base64Decode( const char* pData, uint32_t& iSize );

#endif /* BASE64_H_ */
