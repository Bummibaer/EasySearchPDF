// ConsoleApplication1.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <stdio.h>
#include <string>
//#include <Windows.h>
#include <assert.h>

#include "RaidBuffer.h"
#include "ConvertData.h"
#include "Log.h"


#pragma pack(1)
	typedef union uSet {
		struct sSet {
			BYTE bidA[BID_LENGTH];
			BYTE bidB[BID_LENGTH];
			BYTE pixelC[PIXEL_LENGTH];
			BYTE pixelD[PIXEL_LENGTH];
		} sSet;
		BYTE data[sizeof(sSet)];
	} usSet;


	uSet set = { 0 };

	RaidBuffer *rb = new RaidBuffer(DISC_SIZE, STRIPE_LENGTH, DISC_COUNT);


	int iBurstSize_inBlocks = 125;
	int iBytesofDL;

	bool MockReadFile(int handle, unsigned char  *bytes, int anz);

	bool WriteBootLoader() {
		BYTE bBootLoader[512] = { 0 };
		// Write BootLoader for every disc
		strcpy_s((char*)(bBootLoader + INDEX_OEM), 8, "IPMS_1");						// SN sprintf_s Debug-Version writes  0xFE first !!!!
		*(UINT64 *)(bBootLoader + INDEX_NUMBER_OF_BLOCKS_DL) = sizeof(iBytesofDL) / 512;
		*(UINT64 *)(bBootLoader + INDEX_START_BLOCKS_DL) = 1;
		*(UINT32 *)(bBootLoader + INDEX_NUMBER_OF_STRIPES_DL) = 0x33333333;
		*(UINT32 *)(bBootLoader + INDEX_SAS_BURST_COUNT) = 125;
		*(bBootLoader + INDEX_MARC_CORRUPTED_FS) = 0x55;
		*(UINT32 *)(bBootLoader + INDEX_FS_ID) = 0x66666666;
		strcpy_s((char*)(bBootLoader + INDEX_FS_VER), 8, "IPMS100");
		*(UINT16 *)(bBootLoader + INDEX_BOOT_SIG) = 0x55BB;
		for (int disc = 0; disc < DISC_COUNT; disc++) {
			*(bBootLoader + INDEX_PHYSICAL_DISC_NO) = disc;
			rb->WriteBytes(disc, bBootLoader, sizeof(bBootLoader));
			rb->WriteBytes(disc, bytes, sizeof(bytes));
		}
	}


	void convertData(void) {
		BYTE bytes[STRIPE_LENGTH];
		BYTE *buf = bytes;

		int disk_no = 0;

		int byte_counter = 0;
		int row_counter = 0;
		int block_counter = 0;
		int bdata5_counter = 0;
		bool rc;

		// Write Description List
		// How, yet to communicate with FAU
		// First, write one 64kB with sequential bytes
		UINT8 dl = 0;
		buf = bytes;
		for (int i = 0; i < sizeof bytes; i++) {
			*(buf++) = dl++;
		}

		while (rc = MockReadFile(0, bytes, STRIPE_LENGTH)) {
			if (debug_flags &  D_RUN) printf("CONVERT :  Read %d bytes\n", (int)sizeof(bytes));
			buf = bytes;
			// Write on RAID - Segment
			byte_counter = 0;

			do {
				// Write  BData
				// Write one Exposure Data Line ( two times 2000pix - 256 Bytes)
				if (debug_flags & D_VERBOSE) printf("CONVERT :  Write one Pixel Row %10d\t", row_counter);
				rb->WriteBytes(disk_no, buf, BID_LENGTH);
				buf += BID_LENGTH;

				rb->WriteBytes(disk_no + 8, buf, BID_LENGTH);
				buf += BID_LENGTH;
				assert(*buf == 'C');
				rb->WriteBytes(disk_no, buf, PIXEL_LENGTH);
				if (debug_flags & D_RUN) printf("<%s> to Disc%02d\t", buf, disk_no);
				buf += PIXEL_LENGTH;

				rb->WriteBytes(disk_no + 8, buf, PIXEL_LENGTH);
				if (debug_flags & D_RUN) printf("<%s> to Disc%02d\n", buf, disk_no + 8);
				buf += PIXEL_LENGTH;

				row_counter++;
				block_counter = row_counter / 2;
				byte_counter += 512; // 512 Byte / PixelRow 

				if ((row_counter % 5) == 0) { // every second ( two Bdata5 = one Block)
					bdata5_counter++;
					if (debug_flags & D_RUN) printf("CONVERT :  .............. Wrote one BData5-Block : %10d \t Row: %10d\tBlock = %10d\n",
						bdata5_counter,
						row_counter,
						block_counter);
					if (block_counter >= iBurstSize_inBlocks) {
						row_counter = 0;
						block_counter = 0;
						disk_no++;
						if (disk_no >= 8) {
							printf("CONVERT : one Stripe on all Discs written, press any key\n");
							disk_no = 0;
							UCHAR fbuf[1];
							fread(fbuf, 1, 1, stdin);
						}
						if (debug_flags & D_RUN) printf("CONVERT :  <<<<<<<<<<<<<<< Wrote one Burst , change to disc: %d <<<<<<<<<<<<<<<<<<<<< \n", disk_no);

					}
				}
			} while (byte_counter < STRIPE_LENGTH);
			if (debug_flags & D_RUN) printf("CONVERT :  Read bytes\n");
		}
	}


	bool  MockReadFile(int handle, unsigned char  *bytes, int anz) {
		static uSet set;
		static UINT64 bid = 0;
		static int pixel_row = 0;

		unsigned char *pb = bytes;
		UINT64 ianz = 0;
		UINT64 Bdata5_counter = 0;

		UINT64 bid_null = 0;

		do {
			BYTE *pbid;
			// Write one Bdata
			if (((bid % 5) == 0) || ((bid % 10) == 0)) {
				pbid = (BYTE *)&Bdata5_counter;
				Bdata5_counter++;
			}
			else {
				pbid = (BYTE *)&bid_null;
			}
			// BID A
			for (int i = BID_LENGTH - 1; i >= 0; i--) {
				set.sSet.bidA[i] = *(pbid++);
			}

			// BID B
			for (int i = BID_LENGTH - 1; i >= 0; i--) {
				set.sSet.bidB[i] = set.sSet.bidA[i];
			}
			// Pixel C
			sprintf_s((char *)set.sSet.pixelC, sizeof(set.sSet.pixelC), "C%09d", pixel_row); //6+1+9 = 16
			size_t slen = strlen((char *)set.sSet.pixelC);
			pbid = set.sSet.pixelC + slen + 1;
			for (size_t i = 0; i < PIXEL_LENGTH - slen; i++) {
				*(pbid++) = 0xCC;
			}

			sprintf_s((char *)set.sSet.pixelD, sizeof(set.sSet.pixelD), "D%09d", pixel_row);
			slen = strlen((char *)set.sSet.pixelD);
			pbid = set.sSet.pixelD + slen + 1;
			for (size_t i = 0; i < PIXEL_LENGTH - slen; i++) {
				*(pbid++) = 0xDD;
			}
			pixel_row++;
			bid++;
			ianz += sizeof(set.data);
			memcpy(pb, set.data, sizeof(set.data));
			pb += sizeof(set.data);
		} while (ianz < anz);
		return true;

	}

	UINT64 GenerateDummyContent(UINT8 *buffer, bool bOverwrite, int length) {
		static UINT32 value = 1;
		UINT32 v;
		for (int i = 0; i < length; i += 512)
		{
			if (bOverwrite)
				v = 0xFFFFFFFF;
			else
				v = value;
			for (int j = 0; j < 512; j += 4) {
				buffer[i + j] = ((v >> 24) & 0xFF);
				buffer[i + j + 1] = ((v >> 16) & 0xFF);
				buffer[i + j + 2] = ((v >> 8) & 0xFF);
				buffer[i + j + 3] = (v & 0xFF);
			}
			value++;
		}
		return length;
	}

};