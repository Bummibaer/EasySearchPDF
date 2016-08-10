#pragma once
#ifndef HCONVERTDATA
#define HCONVERTDATA

#include "RaidBuffer.h"
/************************************************************************************
/*	Constants
**************************************************************************************/

typedef enum eList {
	BootLoader,
	DescriptoerList,
	ExposureData
} ;

// BootLoader 
const int INDEX_OEM = 3;
const int INDEX_NUMBER_OF_BLOCKS_DL = 0x24;
const int INDEX_START_BLOCKS_DL = 0x2C;
const int INDEX_NUMBER_OF_STRIPES_DL = 0x34;
const int INDEX_SAS_BURST_COUNT = 0x38;
const int INDEX_PHYSICAL_DISC_NO = 0x40;
const int INDEX_MARC_CORRUPTED_FS = 0x41;
const int INDEX_FS_ID = 0x43;
const int INDEX_FS_VER = 0x52;
const int INDEX_BOOT_SIG = 0x1FE;


// Descriptor List
//

#pragma pack(1)
typedef union uDescList_Header {
	struct sDescList {
		UINT64 Block_No_Next_STRIPE;
		UINT64 Number_of_Blocks;
		UINT64 Number_of_Entries;
		UINT64 Number_of_BID5;
		UINT64 Stripe_ID;
		UINT64 Reserved[3];
	} sDescList;
	BYTE bytes[sizeof(sDescList)];
} tuDescList;
#pragma pack()

const UINT64 DISC_SIZE = (UINT64)1 << 32; 

const int BDATA_LENGTH = 5;
const int BID_LENGTH = 6;
const int PIXEL_LENGTH = 250;
const int BLOCK_LENGTH = 512;
const int BID5 = BDATA_LENGTH * BLOCK_LENGTH;
const int BID10 = 2 * BID5;

UINT64 GenerateDummyContent(UINT8 *buffer, bool bOverwrite, int length);

bool testCalculateData(function_pointer fp);
/************************************************************************************
/*	Functions
**************************************************************************************/

class CalculateData {

public:
	CalculateData();
	CalculateData(bool bRAW, function_pointer fp);	
	bool WriteBootLoader();
	bool WriteDescriptorList();
	void convertData(BYTE bytes[], int length);
	void convertData(void);
};

bool  MockReadFile(int handle, unsigned char  *bytes, int anz);


#endif