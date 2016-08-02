#pragma once
const int D_THREAD =  1;
const int D_RUN = 2;
const int D_SEQUENCE = 4;
const int D_READ = 8;
const int D_WRITE = 16;

const int D_VERBOSE = 0xFF;

const int debug_flags = D_RUN + D_SEQUENCE ;