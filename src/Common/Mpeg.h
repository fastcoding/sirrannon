#ifndef MPEG_H_
#define MPEG_H_

const static uint8_t MPEG_I_FRAME = 1;
const static uint8_t MPEG_P_FRAME = 2;
const static uint8_t MPEG_B_FRAME = 3;

inline frame_t::type MpegToSirannon( uint8_t iField )
{
	switch( iField )
	{
	case MPEG_I_FRAME:
		return frame_t::I;

	case MPEG_P_FRAME:
		return frame_t::P;

	case MPEG_B_FRAME:
		return frame_t::B;

	default:
		return frame_t::other;
	}
}

#endif /* MPEG_H_ */
