/*******************************************************************************************************
*	\file ReadBuffer.cpp
*	$Log$
*	\brief Organizes writes ot a RAID0 Volume
*
*******************************************************************************************************/
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "CopySpecialFiles.h"
#include "Log.h"
#include "RaidBuffer.h"


const int BUF_COUNT = 2;
const char  *filepath = "\\\\smb01\\scratch_21\\netz\\OWC\\";


UINT64 u64DiscSize;
int  iDiscCount;
int  iBurstSize;

enum eDiscState {
	ACTIVE,
	BUSY,
	FULL,
	WRITTEN,
	SPARE,
	PLANNED,
	DISCFILE
} eDiscState;

// static variables

static volatile UINT16 DiscFull = 0;
static UINT16 DiscFullTest = 0;

sBuffer sWriteBuffer;

// RaidBuffer:
BYTE data[BUF_COUNT][DISC_COUNT][STRIPE_LENGTH];

function_pointer fpWriteCB;
bool bRAW = false;

// instance variables

FILE *file[DISC_COUNT];
enum eDiscState  kind[DISC_COUNT];
int current_buf[DISC_COUNT] = { 0 };
size_t head[DISC_COUNT] = { 0 };


bool WriteBytesTo(UINT64 addr, BYTE *buf, size_t length) {
	printf("WriteBytesTo : TODO TODO TODO !\n");
	return false;
}

void BufferWritten(void) {
	DiscFull = 0;
}
bool WritetoFile(int current) {
	bool rc = true;
	if (debug_flags & D_RAID) printf("RAID : Write Buffers to Disc\n");
	if (!bRAW) {
		for (int disc = 0; disc < DISC_COUNT; disc++) {
			if (debug_flags & D_RAID) printf("RAID : \t Write Buffers to File : %d\n", disc);

			size_t s = fwrite(data[current][disc], 1, STRIPE_LENGTH, file[disc]);
			if (s != STRIPE_LENGTH) {
				ErrorPrint(L"fwrite");
				return false;
			}
		}
		BufferWritten();
	}
	else {
		if (debug_flags &  D_THREAD) printf("READC : Set E_READ\n");
		assert(fpWriteCB);
		sWriteBuffer.bytes = (BYTE *)&(data[current]);
		sWriteBuffer.length = DISC_COUNT * STRIPE_LENGTH;
		fpWriteCB(&sWriteBuffer);
		rc = false;
	}
	return rc;
}


void RaidBuffer::SetWriteCallBack(function_pointer *fp) {
	fpWriteCB = *fp;
}

bool RaidBuffer::WriteBytes(int disc, BYTE* buf, size_t length) {
	bool rc = true;
	size_t written = 0;
	if ((head[disc] + length) > STRIPE_LENGTH) {
		written = STRIPE_LENGTH - head[disc];
		memcpy(&data[current_buf[disc]][disc][head[disc]], buf, written);
	}
	else {
		written = length;
		memcpy(&data[current_buf[disc]][disc][head[disc]], buf, length);
	}
	if (debug_flags & D_RAID) printf("RAID : D%d\tWrote %lld Bytes to %0llx\n", disc, written, head[disc]);
	head[disc] += written;
	if (head[disc] >= STRIPE_LENGTH) {
		if (debug_flags & D_RAID) printf("DISC : Buffer of Disc%02d full!\n", disc);
	// @TODO 		assert()

		DiscFull |= (1 << disc);
		if (DiscFull == DiscFullTest) {
			kind[disc] = eDiscState::FULL;
			rc = WritetoFile(current_buf[disc]);
		}
		current_buf[disc]++;
		if (current_buf[disc] == BUF_COUNT) {
			current_buf[disc] = 0;
		}
		head[disc] = 0;
		if (debug_flags & D_RAID) printf("RAID : D%d\tSwitch to buffer %d \n", disc, current_buf[disc]);
		if (written > 0) { // DO REST
			memcpy(&data[current_buf[disc]][disc][head[disc]], buf, length - written);
			if (debug_flags & D_RAID) printf("RAID : D%d\tWrote remainder %lld Bytes to %0llx\n", disc, (length - written), head[disc]);
			head[disc] += (length - written);
		}
	}
	/// TODO handle Buffers, when no Alignement !
	return rc;
}

RaidBuffer::RaidBuffer(bool raw)
{
	bRAW = raw;
	if (!bRAW) {
		char filename[256];
		for (int disc = 0; disc < DISC_COUNT; disc++) {
			DiscFullTest |= 1 << disc;
			sprintf_s(filename, "%sraid_%02d.bin", filepath, disc);
			if (debug_flags & D_RAID) printf("RAID : \t File : %s\n", filename);
			 errno_t err = fopen_s(file+disc , filename, "wb");
			if (err) {
				ErrorPrint(L"fopen");
			}
		}
	}
}

RaidBuffer::~RaidBuffer() {
	printf("RAID : Deconstruct!");
	for (int disc = 0; disc < DISC_COUNT; disc++) {
		int err = fclose(file[disc]);
		if (err) {
			ErrorPrint(L"fclose");
		}
	}

}

