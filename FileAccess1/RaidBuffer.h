#pragma once
class RaidBuffer
{
public:


	RaidBuffer() = delete;
	RaidBuffer(UINT64 size, int stripe, int anz);
	~RaidBuffer();
	bool Add(int disc, BYTE* buf, size_t length);
	bool AddFiles(int disc, const char* file);
	// bool WriteTestFiles();
};

