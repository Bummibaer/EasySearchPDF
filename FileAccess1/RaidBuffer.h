#pragma once


typedef struct  sBuffer {
	BYTE	*bytes;
	long	length;
} sBuffer;

typedef void (*function_pointer) (sBuffer *buffer) ;

const int  DISC_COUNT = 16;
const size_t STRIPE_LENGTH = (UINT32)(1 << 16);

void BufferWritten(void);


class RaidBuffer
{
public:


	RaidBuffer() = delete;
	RaidBuffer(bool raw);
	~RaidBuffer();

	void SetWriteCallBack(function_pointer *fp);
	bool WriteBytes(int disc, BYTE* buf, size_t length);
	// bool WriteTestFiles();

};


