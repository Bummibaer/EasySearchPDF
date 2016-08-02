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
FILE *files[16];
enum eDiscState {
	ACTIVE,
	BUSY,
	FULL,
	WRITTEN,
	SPARE,
	PLANNED
} eDiscState;

static int number = 0;
struct sDisc {

	int disc_no = 0;;
	WCHAR *filename;
	enum eDiscState  kind;
	int current_buf;
	BYTE data[2][STRIPE_LENGTH];
	int head = 0;
	int dest = 0;

	bool Add(BYTE *buf, size_t length) {
		int written = 0;
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
		if (debug_flags & D_RUN) printf("RAID : D%d\tWrite %d from %d Bytes to %0x\n", disc_no,  written, length ,  dest);
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
		size_t s = fwrite(data[current_buf], 1, STRIPE_LENGTH, files[disc_no]);
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
		sDisc();
	}

	~sDisc() {
		printf("RAID : D%d\tDeconstruct\n", disc_no);
		fclose(files[disc_no]);
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


bool RaidBuffer::Add(int disc, BYTE* buf, size_t length) {
	return disks[disc].Add(buf, length);
}

bool RaidBuffer::AddFiles(int disc, const char* file) {
	errno = fopen_s(files + disc, file, "wb");
	if (errno) {
		ErrorPrint(L"fopen");
	}
	return true;
}
RaidBuffer::~RaidBuffer()
{
}

