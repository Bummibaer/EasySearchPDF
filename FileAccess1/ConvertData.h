#pragma once
/************************************************************************************
/*	Constants
**************************************************************************************/

typedef enum eList {
	BootLoader,
	DescriptoerList,
	ExposureData
};

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


const UINT64 DISC_SIZE = (UINT64)1 << 32;
const int DISC_COUNT = 16;
const int STRIPE_LENGTH = 1 << 16;

const int BDATA_LENGTH = 5;
const int BID_LENGTH = 6;
const int PIXEL_LENGTH = 250;

/************************************************************************************
/*	Functions
**************************************************************************************/
void splitData(void);
UINT64 GenerateDummyContent(UINT8 *buffer, bool bOverwrite, int length);