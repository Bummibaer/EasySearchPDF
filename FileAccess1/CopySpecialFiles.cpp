// FileAccess1.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <stdio.h>
#include "winioctl.h"
#include <strsafe.h>
#include <stdlib.h>
#include <chrono>
#include <list>
#include <assert.h>

#include "CopySpecialFiles.h"
#include "ConvertData.h"
#include "Log.h"
#include "Helper.h"
#include "RaidBuffer.h"

void ErrorPrint(LPTSTR lpszFunction);
void logWrite(char* format, ...);
void CreateThreads(void);
bool CreateFiles(wchar_t* filename);

void InitVars(UINT32 blockSize, UINT64 FileSize);
// void trace(const char* format, ...);

void set_event(sBuffer *buffer);

PFN_MYCALLBACK mcbWrite, mcbRead;
FILEACCESS1_API int __stdcall RegisterCallbacks(PFN_MYCALLBACK cbWrite, PFN_MYCALLBACK cbRead) {
	mcbWrite = cbWrite;
	mcbRead = cbRead;
	return 1;
}

const double cscaleMB = 1E6;
const double cscaleGB = 1E9;
const double cscaleTB = 1E12;

enum eProc { F_READ = 0, thread_no_1 = 1, thread_no_2 = 2 };

UINT32	u32BlockSize = 4096;
UINT64 u64FileSize = { 2 << 10 };


static HANDLE	hEventRead;
static HANDLE	hEventWritten;
static HANDLE   hThreadRead;
static HANDLE	hThreadWrite;
static HANDLE   hFile;

DWORD WINAPI ThreadReadProc(LPVOID);
DWORD WINAPI ThreadWriteProc(LPVOID);

BYTE *buffer;
sBuffer *sWriteBuffer;

function_pointer fp;

bool bWriting = false;

const int cTimeout = 10000;

double write_bps_average = 0.0;
double read_bps_average = 0.0;

static bool bRunWrite = true;

const double fCount = (double)std::nano::den;

int debug = 0;
bool bRawFile = false;
bool bOverwrite = false;


FILEACCESS1_API  void setDebug(int value) {
	debug = value;
}

FILEACCESS1_API  void setBlockSize(UINT32 value) {
	printf("BS: %x\t", value);
	u32BlockSize = value;
	printf("%x\n", u32BlockSize);
}
FILEACCESS1_API  void setFileSize(INT64 value) {
	u64FileSize = value;
}
FILEACCESS1_API  void setRawFile(bool value) {
	bRawFile = value;
}


FILEACCESS1_API  void setOverride(bool value) {
	bOverwrite = value;
}

char cAnimation[] = { '|' , '/' ,'-' , '\\' };

sTiming *psTiming;


CalculateData *cd;

FILEACCESS1_API bool  copyFiletoRAW(sTiming *stiming, wchar_t* file) {
	psTiming = stiming;


	InitVars(u32BlockSize, u64FileSize);
	if (bRawFile) {
		bool rc = CreateFiles(file);
		if (!rc) {

			return false;
		}
	}
	if (debug_flags &  D_SEQUENCE) {
		printf("UTB: FS:%15llx\tBS:%12u\n", u64FileSize, u32BlockSize);
		wprintf(L"%s ", file);
		printf("\n");
	}
	fp = set_event;
	cd = new CalculateData(bWriting,fp);

	CreateThreads();

	DWORD dwWaitResult = WaitForSingleObject(
		hThreadWrite,     // array of thread handles
		INFINITE);

	switch (dwWaitResult)
	{
		// All thread objects were signaled
	case WAIT_OBJECT_0:
		if (debug_flags &  D_THREAD) printf("UTB : All threads ended, cleaning up for application exit...\n");
		break;
	case  WAIT_TIMEOUT:
		printf("UTB : Wait Timeout, should not occur !\n");
		exit(2);
		break;
	case WAIT_FAILED:
		printf("WaitForMultipleObjects failed WAIT_FAILED (%d)\n", GetLastError());
		ErrorPrint(L"WAIT_FAILED");
		exit(2);
		// An error occurred
	default:
		printf("WaitForMultipleObjects failed default (%d)\n", GetLastError());
		ErrorPrint(L"WaitForMultipleObjects");
		exit(2);
	}


	if (hFile != NULL)
		CloseHandle(hFile);
	else
		printf("ERROR : File Handle  not closed");


	if (debug_flags &  D_SEQUENCE) printf("UTB : Exit!\n");
	return true;
}

void InitVars(UINT32 blockSize, UINT64 FileSize) {


	psTiming->dReadElapsed = 0.0;
	psTiming->dWriteElapsed = 0;

	u32BlockSize = blockSize;
	u64FileSize = FileSize;

	write_bps_average = 0.0;
	read_bps_average = 0.0;

	buffer = (BYTE *)malloc(blockSize);
	bRunWrite = true;
}

bool  CreateFiles(wchar_t* filename)
{
	bool rc = true;
	if (debug_flags &  D_RUN) printf("before open\n");
	hFile = CreateFileW(
		filename,          // drive to open
		FILE_GENERIC_WRITE,
		0,
		NULL,             // default security attributes
						  //CREATE_ALWAYS,
		OPEN_EXISTING,    // disposition
		FILE_FLAG_OVERLAPPED,                // file attributes
		NULL);            // do not copy file attributes
	if (debug_flags &  D_RUN) printf("after open\n");

	if (hFile == INVALID_HANDLE_VALUE)    // cannot open the drive
	{
		wchar_t wc[128] = L"WRITE: Tried to open : ";
		lstrcatW(wc, filename);
		wprintf(L"%s\n", wc);
		OutputDebugString(wc);
		ErrorPrint(L"CreateFileW");
		rc = false;
		return false;
	}
	if (debug_flags &  D_SEQUENCE) wprintf(L"WRITE : %s opened !\n", filename);

	if (debug_flags &  D_SEQUENCE) printf("INFO: Files created\n");

	return rc;
}

void  CreateThreads(void) {
	DWORD(__stdcall *t)(LPVOID);

	// Read Thread
	t = ThreadReadProc;
	DWORD threadID = 0;
	hThreadRead = CreateThread(
		NULL,              // default security
		0,                 // default stack size
		t,                // Pointer to  the thread function
		NULL,              // Anzahl of Write Threads
		CREATE_SUSPENDED,    // start after Write Threads are created !
		&threadID
		);

	// Write Threads
	hEventRead = CreateEvent(
		NULL,    // default security attribute 
		FALSE,    // manual-reset event 
		FALSE,    // initial state = Written signaled, Read not
		NULL);   // unnamed event object 
	hEventWritten = CreateEvent(
		NULL,    // default security attribute 
		FALSE,    // manual-reset event 
		TRUE,    // initial state = Written signaled, Read not
		NULL);   // unnamed event object 

	if (hEventWritten == NULL)
	{
		printf("CreateEvent failed with %d.\n", GetLastError());
		return;
	}
	t = ThreadWriteProc;
	threadID++;
	hThreadWrite = CreateThread(
		NULL,              // default security
		0,                 // default stack size
		t,        // name of the thread function
		NULL,              // no thread parameters
		0,                 // default startup flags
		&threadID);

	if (hThreadWrite == NULL)
	{
		printf("CreateThread failed (%d)\n", GetLastError());
		return;
	}


	ResumeThread(hThreadRead);

	if (debug_flags &  D_RUN) printf("INIT : Threads & Events created!\n");

}

void set_event(sBuffer *buffer) {
	sWriteBuffer = buffer;
	bWriting = true;
	SetEvent(hEventWritten);
}

DWORD WINAPI ThreadReadProc(LPVOID lpParam)
{
	// lpParam not used in this example.

	UINT64 newAddress = 0;

	int read_buffer = 0;
	DWORD dwWaitResult;

	double t_bytes_per_second, t_bytes_per_ns;
	double duration_sec;

	bool bAllRead = false;

	std::chrono::time_point<std::chrono::high_resolution_clock>   start, end;

	while (!bAllRead) {
		if (debug_flags &  D_THREAD) printf("READC : --------------- Wait for Write handles\n");
		dwWaitResult = WaitForSingleObject(
			hEventWritten, // event handle
			INFINITE /*cTimeout*/);    // indefinite wait
		switch (dwWaitResult)
		{
			// Event object was signaled
		case WAIT_OBJECT_0:
		{
			if (debug_flags &  D_THREAD) printf("READC : ---------------------All Writes handled!\n");
			while (!bWriting) {
				//GenerateDummyContent(buffer.bytes[read_buffer], bOverwrite, u32BlockSize);
				bool rc = MockReadFile(0, buffer, u32BlockSize);
				if (debug_flags & D_READ) printf("READC : %d bytes read!\n",u32BlockSize);
				cd->convertData(buffer, u32BlockSize);
			}

				fflush(stdout);
		}
		break;
		case WAIT_TIMEOUT:
			printf("READ : Timeout!\n");
			break;
		case WAIT_FAILED:
			ErrorPrint(L"READ : WAIT_READ WAIT_FAILED");
			break;
			// An error occurred
		default:
			ErrorPrint(L"READ : WAIT_READ UNKNOWN");
			break;
		} // switch GetLastError()

	};
	bRunWrite = false;

	if (debug_flags &  D_RUN) printf("CREATECONTENT : End of Thread\n");
	return 1;
}


DWORD WINAPI ThreadWriteProc(LPVOID lpParam)
{
	// lpParam not used in this example.
	DWORD dwWaitResult, dwError;
	DWORD dwBytesWritten;
	int write_buffer = 0;
	double t_bytes_per_second, t_bytes_per_ns;
	double duration_sec;
	int iPercent = 1;
	double dPercent;
	double max_duration = 0.0;
	BOOL rc;
	UINT64 newAddress = 0;
	int thread_no = GetCurrentThreadId();

	OVERLAPPED stOverlapped = { 0 };

	stOverlapped.hEvent = CreateEvent(
		NULL,    // default security attribute 
		TRUE,    // manual-reset event 
		FALSE,    // initial state = Written signaled, Read not
		NULL);

	int aIndex = 0;
	//SN if (debug == 0) printf("%c", cAnimation[aIndex++]);
	if (debug_flags &  D_THREAD) printf("WRITE : Thread no. %d starts \n", thread_no);
	do {
		if (debug_flags &  D_THREAD) printf("WRITE : T%d waiting for Read event...\n", thread_no);
		dwWaitResult = WaitForSingleObject(
			hEventRead, // event handle
			cTimeout);    // indefinite wait
		if (debug_flags &  D_THREAD) printf("WRITE : T%d Got Read Event Write \n", thread_no);

		switch (dwWaitResult) { // Event object was signaled
		case WAIT_OBJECT_0:
		{
			if (debug_flags &  D_WRITE) printf("%*sWRITE : T%d\tWriting %d bytes from buf%d to %0llx\n", (thread_no + 1) * 10, " ", thread_no, sWriteBuffer->length, write_buffer, newAddress);

			auto start = std::chrono::high_resolution_clock::now();
			rc = WriteFile(hFile, sWriteBuffer->bytes, sWriteBuffer->length, NULL, &stOverlapped);
			if (!rc)
			{
				switch (dwError = GetLastError()) {
				case  ERROR_IO_PENDING:
				{
					dwWaitResult = WaitForSingleObject(stOverlapped.hEvent, INFINITE);
					auto end = std::chrono::high_resolution_clock::now();
					switch (dwWaitResult) {
					case  WAIT_OBJECT_0:
						BufferWritten();
						rc = GetOverlappedResult(hFile, &stOverlapped, &dwBytesWritten, TRUE);
						printf("WRITE : rc=%0x %0x\n", rc, dwBytesWritten);
						if (rc) {
							assert(dwBytesWritten == sWriteBuffer->length);
							if (debug_flags &  D_WRITE) printf("%*sWRITE : T%d\tEvent written\n", (thread_no + 1) * 10, " ", thread_no);

							// Send Signal to Read
							SetEvent(hEventWritten);

							auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

							duration_sec = (double)duration_ns.count() / (double)1E9;
							t_bytes_per_ns = (double)sWriteBuffer->length / duration_ns.count();
							t_bytes_per_second = t_bytes_per_ns  * 1E9;
							psTiming->dWriteElapsed += duration_sec;

							newAddress += dwBytesWritten;
							stOverlapped.OffsetHigh = (newAddress >> 32) & 0xFFFFFFFF;
							stOverlapped.Offset = newAddress & 0xFFFFFFFF;
							write_bps_average = (double)stOverlapped.Offset / psTiming->dWriteElapsed / 1E9;

							max_duration = max(max_duration, duration_sec);
							if (debug_flags &  D_RUN) printf("WRITE : D:%lldns\t%6.3fGB/sec\tE:%fms  with %6.3fGB/sec,%llx\n",
								duration_ns.count(),
								t_bytes_per_ns,
								psTiming->dWriteElapsed,
								write_bps_average,
								newAddress
								);
							mcbWrite(t_bytes_per_second / 1E6);   // MB/sec
							if (debug_flags &  D_RUN) {
								dPercent = (double)stOverlapped.Offset / (double)u64FileSize * 100.0;
								if (dPercent >= iPercent) {
									//SNprintf("\b%d ", thread_no);
									printf("%d", thread_no);
									iPercent++;
								}
								else {
									//SNprintf("\b%c", cAnimation[aIndex++]);
									//SNif (aIndex >= sizeof(cAnimation)) aIndex = 0;
								}
							}
							if (debug_flags &  D_VERBOSE) printf("\nWRITE : T%d ----------------- bRunWrite %d , offset = %0llx---------------\n",
								thread_no, bRunWrite, newAddress);
							if (newAddress >= u64FileSize) {
								if (debug_flags &  D_WRITE) printf("WRITE : T%d finisch Writing\n", thread_no);
								break;
							}
							write_buffer++;
							if (write_buffer == cBuffers) {
								write_buffer = 0;
							}
							fflush(stdout);
						}
						else
						{
							switch (dwError = GetLastError())
							{
							case ERROR_HANDLE_EOF:
								if (debug > 0) printf("WRITE : EOF found !\n");
								bRunWrite = FALSE;
								break;
							case ERROR_IO_INCOMPLETE:
								// Operation is still pending, allow while loop
								// to loop again after printing a little progress.
								printf("WRITE : GetOverlappedResult I/O Incomplete\n");
								break;
							default:
								printf("WRITE : T%d Default , ERROR !\n", thread_no);
								ErrorPrint(L"getOverlappedResult");
								break; // case ERROR_IO_PENDING
							}
						} // rc 
						break;
					case WAIT_TIMEOUT:
						printf("WRITE :  Timeout! \n");
						OutputDebugString(L"WRITE TIMEOUT");
						break;
					case WAIT_FAILED:
						ErrorPrint(L"WAIT_READ WAIT_FAILED");
						bRunWrite = false;
						break;
						// An error occurred
					default:
						ErrorPrint(L"WAIT Wait For Single Object");
						bRunWrite = false;
						break; // case WAIT_OBJECT

					} // switch  Wait Write
				} // Case ERROR_IO_PENDING
				break;
				default:
					printf("WRITE : Error WriteFile");
					ErrorPrint(L"WriteFile");
					break;

				}// switch Get Error Write File
			} // rc WriteFile
			else {
				printf("WRITE : Error WriteFile");
				ErrorPrint(L"WriteFile");
				break;

			}
		}  // case WAIT_OB
		break;
		case WAIT_TIMEOUT:
			printf("WRITE : T%d Timeout! \n", thread_no);
			OutputDebugString(L"WRITE TIMEOUT");
			break;
		case WAIT_FAILED:
			ErrorPrint(L"WAIT_READ WAIT_FAILED");
			bRunWrite = false;
			break;
			// An error occurred
		default:
			ErrorPrint(L"WAIT Wait For Single Object");
			bRunWrite = false;
			break; // case WAIT_OBJECT
		} // WAIT_OBJECT_0 Wait READ
	} while (newAddress < u64FileSize);

	if (debug_flags &  D_WRITE) printf("\nWRITE T%d  Write End Offset : %llx\n", thread_no, newAddress);
	// Now that we are done reading the buffer, we could use another
	// event to signal that this thread is no longer reading. This
	// example simply uses the thread handle for synchronization (the
	// handle is signaled when the thread terminates.
	sWriteBuffer->bytes[0] = 'E';
	sWriteBuffer->bytes[1] = 'N';
	sWriteBuffer->bytes[2] = 'D';
	sWriteBuffer->bytes[3] = '\0';
	rc = WriteFile(hFile, sWriteBuffer->bytes, 512, NULL, &stOverlapped);
	if (!rc) {
		switch (GetLastError()) {
		case  ERROR_IO_PENDING:
			dwWaitResult = WaitForSingleObject(stOverlapped.hEvent, INFINITE);
			switch (dwWaitResult) {
			case  WAIT_OBJECT_0:
				rc = GetOverlappedResult(hFile, &stOverlapped, &dwBytesWritten, TRUE);
				if (!rc) {
					switch (dwError = GetLastError()) {
					case ERROR_HANDLE_EOF:
						if (debug_flags &  D_WRITE) printf("WRITE : EOF found !\n");
						bRunWrite = FALSE;
						break;
					case ERROR_IO_INCOMPLETE:
						// Operation is still pending, allow while loop
						// to loop again after printing a little progress.
						printf("WRITE : GetOverlappedResult I/O Incomplete\n");
						break;
					case ERROR_INVALID_PARAMETER:
						printf("WRITE : Wrong Parameter in GetOverlappedResult\n");
						assert(false);
						break;
					default:
						printf("WRITE : GetOverLapped Unknown Error %x\n", dwError);
						break;
					} // switch  last Error
				}
				else {
					if (dwBytesWritten != 512) {
						printf("written : %d  to %x\n", dwBytesWritten, stOverlapped.Offset);
						ErrorPrint(L"Write End");
						assert(dwBytesWritten == 4);
					}
				}
			default:
				//printf("WRITE : default WaitForSingleObject \n");
				break;
			} // switch Result Wait for Write
		default:

			rc = GetOverlappedResult(hFile, &stOverlapped, &dwBytesWritten, TRUE);
			if (!rc) {
				switch (dwError = GetLastError()) {
				case ERROR_HANDLE_EOF:
					if (debug_flags &  D_WRITE) printf("WRITE : EOF found !\n");
					bRunWrite = FALSE;
					break;
				case ERROR_IO_INCOMPLETE:
					// Operation is still pending, allow while loop
					// to loop again after printing a little progress.
					printf("WRITE : GetOverlappedResult I/O Incomplete\n");
					break;
				case ERROR_INVALID_PARAMETER:
					printf("WRITE : Wrong Parameter in GetOverlappedResult\n");
					assert(false);
					break;
				default:
					printf("WRITE : GetOverLapped Unknown Error %x\n", dwError);
					break;
				} // switch  last Error
			}
			else {
				if (dwBytesWritten != 512) {
					printf("written : %d  to %x\n", dwBytesWritten, stOverlapped.Offset);
					ErrorPrint(L"Write End");
					assert(dwBytesWritten == 512);
				}
				if (debug_flags &  D_WRITE) printf("WRITE : T%d END written\n", thread_no);

			}
			break;
		} // switch  Error WriteFile
	} // rc
	else {
		printf("WRITE : rc == true\n");
		ErrorPrint(L"Write End");

	}

	if (debug_flags &  D_THREAD)
		printf("WRITE: Thread %d exiting MAX_DURATION = %f \n", GetCurrentThreadId(), max_duration);
	else
		printf("\n");
	return 1;
}



void ErrorPrint(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		LPWSTR(&lpMsgBuf),
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);

	wprintf(L"%s\n", (wchar_t *)lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

}

FILEACCESS1_API void testSSDRAID()
{
	// splitData();
	printf("Ende \n");
}

// This is an example of an exported function.
FILEACCESS1_API void  getFileSystem(wchar_t* wszPath, wchar_t* nameOfFS)
{
	HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
	static FILE_SYSTEM_RECOGNITION_INFORMATION fsri_s;
	DWORD BytesReturned;

	wprintf(L"Path : %ls\n", wszPath);

	hDevice = CreateFileW(
		wszPath,          // drive to open
		FILE_READ_ATTRIBUTES | SYNCHRONIZE | FILE_TRAVERSE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL);            // do not copy file attributes

	if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
	{
		ErrorPrint(wszPath);
		goto exit;
	}

	BOOL rc = DeviceIoControl(
		hDevice,             // handle to volume
		FSCTL_QUERY_FILE_SYSTEM_RECOGNITION, // dwIoControlCode
		NULL,                         // lpInBuffer
		0,                            // nInBufferSize
		fsri_s.FileSystem,         // output buffer
		sizeof(FILE_SYSTEM_RECOGNITION_INFORMATION),       // size of output buffer
		&BytesReturned,    // number of bytes returned
		(LPOVERLAPPED)0   // OVERLAPPED structure
		);
	if (!rc)
	{
		ErrorPrint(wszPath);
		goto exit;
	}
	wprintf(L"succeeded.\n\n");

	wprintf(L"FSCTL_QUERY_FILE_SYSTEM_RECOGNITION returned success.\n");

	wprintf(L"FSCTL_QUERY_FILE_SYSTEM_RECOGNITION retrieved \"%S\".\n",
		fsri_s.FileSystem);	size_t length = strlen(fsri_s.FileSystem);
	wchar_t text_wchar[30];

	mbstowcs_s(&length, text_wchar, fsri_s.FileSystem, 30);

	nameOfFS = text_wchar;

exit: CloseHandle(hDevice);
}

bool VerifyFile(wchar_t* filename) {
	HANDLE hFile;
	BOOL rc;
	DWORD dwBytesRead;
	UINT64 llOffset = { 0 };
	wprintf(L"VERIFIER : File %s \n", filename);

	hFile = CreateFileW(
		filename,          // drive to open
		FILE_GENERIC_READ,
		0,
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		FILE_FLAG_SEQUENTIAL_SCAN,                // file attributes
		NULL);            // do not copy file attributes

	if (hFile == INVALID_HANDLE_VALUE)    // cannot open the drive
	{
		wchar_t wc[128] = L"READ: Tried to open";
		lstrcatW(wc, filename);
		wprintf(L"%s\n", wc);
		ErrorPrint(L"CreateFileW");
		return false;
	}

	wprintf(L"VERIFIER : %s opened !\n", filename);
	sWriteBuffer->bytes = (UCHAR*)malloc(u32BlockSize);
	ULONG32 value = 1;

	if (!bRawFile) {
		LARGE_INTEGER li;
		GetFileSizeEx(hFile, &li);
		if (li.QuadPart != (u64FileSize + 0x200)) {
			printf("VERIFIER Filesizes differ : %0llx - %0llx\n", li.QuadPart, u64FileSize);
			CloseHandle(hFile);
			return false;
		}
	}
	std::list<int> ilErrors;
	do {
		rc = ReadFile(hFile, sWriteBuffer->bytes, u32BlockSize, &dwBytesRead, NULL);
		if (rc) {
			for (UINT32 block = 0; block < dwBytesRead; block += 512) {
				if (bOverwrite) value = 0xFFFFFFFF;
				ilErrors.clear();
				for (UINT32 index = 0; index < 512; index += 4) {
					UINT32 act_value = (sWriteBuffer->bytes[block + index] << 24) |
						(sWriteBuffer->bytes[block + index + 1] << 16) |
						(sWriteBuffer->bytes[block + index + 2] << 8) |
						sWriteBuffer->bytes[block + index + 3]
						;
					if (act_value != value) {
						ilErrors.push_back(index);
						//printf("@%d %0x\t%0x\n", index,act_value, v);
					}
				}
				if (ilErrors.size() > 0) {
					printf("VERIFIER : Errors on Address: %0llx\tblock: %08x\t :", llOffset, block);
					// fread(input, 1, 1, stdin);
					for (auto c : ilErrors) {
						printf("%d ", c);
					}
					printf("\n");
				}
				value++;
			}
		}
		else
		{
			ErrorPrint(L"VERIFIER ReadFile");
		}
		llOffset += dwBytesRead;
		// printf("VERIFIER : offset %llx\n", llOffset);
	} while (llOffset < u64FileSize);

	rc = ReadFile(hFile, sWriteBuffer->bytes, 512, &dwBytesRead, NULL);
	if (rc) {
		int rc = strncmp((const char *)sWriteBuffer->bytes, "END", 4);
		if (rc != 0) {
			wprintf(L"VERIFIER : File END of %s is not O.K.\n", filename);
			printf("VERIFIER : Read %02x %02x %02x %02x\n",
				sWriteBuffer->bytes[0],
				sWriteBuffer->bytes[1],
				sWriteBuffer->bytes[2],
				sWriteBuffer->bytes[3]
				);
		}
		else {
			printf("VERIFIER Test O.K.\n");
		}
	}
	else
	{
		ErrorPrint(L"VERIFIER ReadFile");
	}

	free(sWriteBuffer->bytes);
	CloseHandle(hFile);
	return rc ? false : true;
}

/*
void trace(const char* format, ...)
{
	WCHAR buffer[1000] = L"";
	WCHAR wFormat[1000] = L"";
	size_t retlen;
	mbstowcs_s(&retlen, wFormat,sizeof(wFormat), format, strlen(format+1));
	va_list argptr;
	va_start(argptr, format);
	wvsprintf(buffer, wFormat, argptr);
	va_end(argptr);
	printf("retlen=%d\n", retlen);

	OutputDebugString(buffer);
}
*/

void printc() {

	printf("%sThis text is RED!%s\n", ANSI_COLOR_RED, ANSI_COLOR_RESET);
	printf(ANSI_COLOR_GREEN   "This text is GREEN!"   ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_YELLOW  "This text is YELLOW!"  ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_BLUE    "This text is BLUE!"    ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_MAGENTA "This text is MAGENTA!" ANSI_COLOR_RESET "\n");
	printf(ANSI_COLOR_CYAN    "This text is CYAN!"    ANSI_COLOR_RESET "\n");

}
/*
void testSetPointer(long addr) {
	printf("SetFilePointer to \n");
	SetFilePointer(hFiles[0], addr&0xFFFFFFFF, (addr>32)& 0xFFFFFFFF, FILE_BEGIN);
	printf("SetFilePointer to \n");
	WriteFile(hFiles[0], )
		return true;


}
*/