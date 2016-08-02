// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the FILEACCESS1_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// FILEACCESS1_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef FILEACCESS1_EXPORTS
#define FILEACCESS1_API __declspec(dllexport)
#else
#define FILEACCESS1_API __declspec(dllimport)
#endif


extern "C" {

	const int	cBuffers = 2;
	const int   cMaxFile = 16;

	typedef struct sTiming
	{
		double dReadElapsed;
		double dWriteElapsed[cMaxFile];
	} sTiming;

	FILEACCESS1_API bool  copyFiletoRAW(sTiming *sTiming , int anz , wchar_t* file[]);

	FILEACCESS1_API  void setNegate(bool value);
	FILEACCESS1_API void setDebug(int value);
	FILEACCESS1_API void setBlockSize(UINT32 value);
	FILEACCESS1_API void setFileSize(INT64 value);
	FILEACCESS1_API void setRawFile(bool value);
	FILEACCESS1_API void setOverride(bool value);

	FILEACCESS1_API void  testSSDRAID();


	typedef void(__stdcall *PFN_MYCALLBACK)(double tWrite);

	FILEACCESS1_API int __stdcall RegisterCallbacks(PFN_MYCALLBACK cbWrite, PFN_MYCALLBACK cbRead);

	FILEACCESS1_API bool VerifyFile(wchar_t* filename);

	void ErrorPrint(LPTSTR lpszFunction);

}