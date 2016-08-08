#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "Log.h"
#include "RaidBuffer.h"
#include "CopySpecialFiles.h"
const int   DISC_COUNT = 16;
const UINT32 STRIPE_LENGTH = (UINT32)(1 << 16);

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

static int number = 0;
struct sDisc {
	// Parameter
	bool bRAW = false;

	int disc_no = 0;;
	WCHAR *filename;
	FILE *file;
	enum eDiscState  kind;
	int current_buf;
	BYTE data[2][STRIPE_LENGTH];
	size_t head = 0;
	size_t dest = 0;

	bool WriteBytesTo(UINT64 addr, BYTE *buf, size_t length) {
		return false;
	}
	bool WriteBytes(BYTE *buf, size_t length) {
		size_t written = 0;
		dest = head;
		if ((head + length) > STRIPE_LENGTH) {
			written = STRIPE_LENGTH - head;
			memcpy(&data[current_buf][head], buf, written);
		}
		else {
			written = length;
			memcpy(&data[current_buf][head], buf, length);
		}
		head += written;
		if (debug_flags & D_RUN) printf("RAID : D%d\tWrite %lld from %lld Bytes to %0llx\n", disc_no, written, length, dest);
		if (head >= STRIPE_LENGTH) {
			if (debug_flags & D_RUN) printf("DISC : Buffer of Disc%02d full, switch to other Buffer !\n", disc_no);
			kind = eDiscState::FULL;
			WritetoFile();
			current_buf = current_buf == 0 ? 1 : 0;
			head = 0;
			printf("RAID : D%d\tSwitch to buffer %d \n", disc_no, current_buf);
			if (written > 0) { // DO REST
				memcpy(&data[current_buf][head], buf, length - written);
				head += (length - written);
			}
		}
		/// TODO handle Buffers, when no Alignement !
		return true;
	}

	bool WritetoFile(void) {
		printf("RAID : Write Buffer to Disc%02d\n", disc_no);
		size_t s = fwrite(data[current_buf], 1, STRIPE_LENGTH, file);
		if (s != STRIPE_LENGTH) {
			ErrorPrint(L"fwrite");
			return false;
		}
		kind = eDiscState::ACTIVE;
		return true;
	}

	sDisc() {
		disc_no = number++;
		kind = eDiscState::ACTIVE;
		current_buf = 0;
		printf("RAID : Init sDisc !\n");
	}
	sDisc(bool bFile, const char* fFile)
	{
		printf("RAID : Init sDisc with File !\n");
		errno = fopen_s(&file, fFile, "wb");
		if (errno) {
			ErrorPrint(L"fopen");
		}

		sDisc();
	}

	~sDisc() {
		disc_no = number++;
		printf("RAID : D%d\tDeconstruct\n", disc_no);
		fclose(file);
	}
} disks[DISC_COUNT];

RaidBuffer::RaidBuffer(UINT64 size, int stripe, int anz)
{
	u64DiscSize = size;
	char  buf[255];
	for (int i = 0; i < anz; i++) {
		sprintf_s(buf, sizeof(buf), "\\\\smb01\\scratch_21\\netz\\OWC\\test.%02d", i);
		AddFiles(i, buf);
	}
	// others not used yet
}


bool RaidBuffer::WriteBytes(BYTE* buf, size_t length) {
	return disks[disc].WriteBytes(buf, length);
}

RaidBuffer::~RaidBuffer()
{
}

