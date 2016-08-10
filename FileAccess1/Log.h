#pragma once
const UINT32 D_THREAD =  1;
const UINT32 D_RUN = 2;
const UINT32 D_SEQUENCE = 4;
const UINT32 D_READ = 8;
const UINT32 D_RAID = 16;
const UINT32 D_WRITE = 32;

const UINT32 D_VERBOSE = 0xFF;

const UINT32 debug_flags = D_VERBOSE; //  D_RUN + D_SEQUENCE + D_RAID;