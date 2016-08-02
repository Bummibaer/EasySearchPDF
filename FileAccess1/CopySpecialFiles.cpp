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

void ErrorPrint(LPTSTR lpszFunction);
void logWrite(char* format, ...);
void CreateThreads(int anz);
bool CreateFiles(int anz, wchar_t* file1, wchar_t *files2, bool bRAW);
void InitVars(UINT32 blockSize, UINT64 FileSize, int iAnz);
// void trace(const char* format, ...);


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


static HANDLE	hEventRead[cMaxFile];
static HANDLE	hEventWritten[cMaxFile];
static HANDLE   hThreadRead;
static HANDLE	hThreads[cMaxFile];
static HANDLE	hFiles[cMaxFile];


DWORD WINAPI ThreadReadProc(LPVOID);
DWORD WINAPI ThreadWriteProc(LPVOID);

const int cTimeout = 10000;

struct sBuffer {
	UCHAR	*bytes[cBuffers];
	DWORD	length[cBuffers];
} buffer;

bool bNegated = false;



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
	u32BlockSize = value;
}
FILEACCESS1_API  void setFileSize(INT64 value) {
	u64FileSize = value;
}
FILEACCESS1_API  void setRawFile(bool value) {
	bRawFile = value;
}

FILEACCESS1_API  void setNegated(bool value) {
	bNegated = value;
}

FILEACCESS1_API  void setOverride(bool value) {
	bOverwrite = value;
}

char cAnimation[] = { '|' , '/' ,'-' , '\\' };

sTiming *psTiming;
int iAnz;

#include "Helper.h"
FILEACCESS1_API bool  copyFiletoRAW(sTiming *stiming, int anz, wchar_t* file[]) {
	psTiming = stiming;
	iAnz = anz;

	InitVars(u32BlockSize, u64FileSize, iAnz);

	bool rc = CreateFiles(iAnz, file[0], file[1], bRawFile);
	if (!rc) {

		return false;
	}

	if (debug_flags &  D_SEQUENCE) {
		printf("UTB: FS:%15llx\tBS:%12u\n", u64FileSize, u32BlockSize);
		for (int i = 0; i < iAnz; i++)
			wprintf(L"%s ", file[i]);
		printf("\n");
	}
	CreateThreads(iAnz);

	DWORD dwWaitResult = WaitForMultipleObjects(
		iAnz,   // number of handles in array
		hThreads,     // array of thread handles
		TRUE,          // wait until all are signaled
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


	for (int i = 0; i < iAnz; i++) {
		if (hFiles[i] != NULL)
			CloseHandle(hFiles[i]);
		else
			printf("Handle %d not closed", i);
	}

	if (debug_flags &  D_SEQUENCE) printf("UTB : Exit!\n");
	return true;
}

void InitVars(UINT32 blockSize, UINT64 FileSize, int iAnz) {


	psTiming->dReadElapsed = 0.0;
	for (int i = 0; i < iAnz; i++) psTiming->dWriteElapsed[i] = 0;

	u32BlockSize = blockSize;
	u64FileSize = FileSize;

	write_bps_average = 0.0;
	read_bps_average = 0.0;

	buffer.bytes[0] = (UCHAR*)malloc(u32BlockSize);
	if (buffer.bytes[0] == NULL) {
		printf("Could not alloc memory ! buffer.bytes[0]");
		exit(3);
	}
	buffer.bytes[1] = (UCHAR*)malloc(u32BlockSize);
	if (buffer.bytes[1] == NULL) {
		printf("Could not alloc memory ! buffer.bytes[1]");
		exit(3);
	}

	bRunWrite = true;

}

bool  CreateFiles(int anz, wchar_t* file1, wchar_t *file2, bool bRAW)
{
	bool rc = true;
	wchar_t *filenames[2];
	filenames[0] = file1;
	filenames[1] = file2;
	for (int i = 0; i < anz; i++) {
		if (debug_flags &  D_RUN ) printf("before open\n");
		hFiles[i] = CreateFileW(
			filenames[i],          // drive to open
			FILE_GENERIC_WRITE,
			0,
			NULL,             // default security attributes
							  //CREATE_ALWAYS,
			bRawFile ? OPEN_EXISTING : CREATE_ALWAYS,    // disposition
			FILE_FLAG_OVERLAPPED,                // file attributes
			NULL);            // do not copy file attributes
		if (debug_flags &  D_RUN) printf("after open\n");

		if (hFiles[thread_no_1] == INVALID_HANDLE_VALUE)    // cannot open the drive
		{
			wchar_t wc[128] = L"WRITE: Tried to open : ";
			lstrcatW(wc, filenames[i]);
			wprintf(L"%s\n", wc);
			OutputDebugString(wc);
			ErrorPrint(L"CreateFileW");
			rc = false;
			if (file1 != NULL) CloseHandle(&filenames[i]);
			return false;
		}
		if (debug_flags &  D_SEQUENCE) wprintf(L"WRITE : %s opened !\n", filenames[i]);

	}
	if (debug_flags &  D_SEQUENCE ) printf("INFO: Files created\n");

	return rc;
}
DWORD threadID[cMaxFile + 1];
int threadNo[cMaxFile + 1];

void  CreateThreads(int anz) {
	DWORD(__stdcall *t)(LPVOID);
	static int iAnz = anz;

	// Read Thread
	t = ThreadReadProc;
	threadID[0] = 0;
	hThreadRead = CreateThread(
		NULL,              // default security
		0,                 // default stack size
		t,                // Pointer to  the thread function
		&iAnz,              // Anzahl of Write Threads
		CREATE_SUSPENDED,    // start after Write Threads are created !
		&threadID[0]
		);

	// Write Threads
	for (int i = 0; i < anz; i++) {
		hEventRead[i] = CreateEvent(
			NULL,    // default security attribute 
			FALSE,    // manual-reset event 
			FALSE,    // initial state = Written signaled, Read not
			NULL);   // unnamed event object 
		hEventWritten[i] = CreateEvent(
			NULL,    // default security attribute 
			FALSE,    // manual-reset event 
			TRUE,    // initial state = Written signaled, Read not
			NULL);   // unnamed event object 

		if (hEventWritten[i] == NULL)
		{
			printf("CreateEvent failed with %d.\n", GetLastError());
			return;
		}
		t = ThreadWriteProc;
		threadNo[i] = i;

		hThreads[i] = CreateThread(
			NULL,              // default security
			0,                 // default stack size
			t,        // name of the thread function
			threadNo + i,              // no thread parameters
			0,                 // default startup flags
			&threadID[i + 1]);

		if (hThreads[i] == NULL)
		{
			printf("CreateThread failed (%d)\n", GetLastError());
			return;
		}
	}

	ResumeThread(hThreadRead);

	if (debug_flags &  D_RUN) printf("INIT : Threads & Events created!\n");

}


DWORD WINAPI ThreadReadProc(LPVOID lpParam)
{
	// lpParam not used in this example.
	int iAnzWriteThreads = *(int *)lpParam;
	UINT64 newAddress = 0;

	int read_buffer = 0;
	DWORD dwWaitResult;

	double t_bytes_per_second, t_bytes_per_ns;
	double duration_sec;

	bool bAllRead = false;

	GenerateDummyContent(buffer.bytes[read_buffer], bOverwrite, u32BlockSize);

	newAddress += u32BlockSize;
	buffer.length[read_buffer] = u32BlockSize;
	if (debug_flags &  D_THREAD) printf("READC : Set E_READ\n");

	for (int i = 0; i < iAnz; i++) SetEvent(hEventRead[i]);

	if (debug_flags &  D_RUN)
		printf("READC : Read %d bytes to Buf%d \t%llx\t%llx\n",
			buffer.length[read_buffer],
			read_buffer,
			newAddress,
			u64FileSize
			);

	read_buffer++;
	std::chrono::time_point<std::chrono::high_resolution_clock>   start, end;

	while (!bAllRead) {
		if (debug_flags &  D_THREAD) printf("READC : --------------- Wait for Write handles\n");
		dwWaitResult = WaitForMultipleObjects(
			iAnzWriteThreads,
			hEventWritten, // event handle
			TRUE,
			INFINITE /*cTimeout*/);    // indefinite wait
		switch (dwWaitResult)
		{
			// Event object was signaled
		case WAIT_OBJECT_0:
		{
			if (debug_flags &  D_THREAD) printf("READC : ---------------------All Writes handled!\n");
			if (newAddress >= u64FileSize) { // All Written !
				if (debug_flags &  D_RUN) printf("READC : All written!\n");
				if (debug_flags &  D_THREAD ) printf("READC : Set E_READ\n");
				for (int i = 0; i < iAnz; i++) SetEvent(hEventRead[i]);
				bAllRead = true;
				break;
			}
			auto start = std::chrono::high_resolution_clock::now();
			GenerateDummyContent(buffer.bytes[read_buffer], bOverwrite, u32BlockSize);

			auto end = std::chrono::high_resolution_clock::now();
			buffer.length[read_buffer] = u32BlockSize;
			if (debug_flags &  D_THREAD) printf("READC : Set E_READ\n");
			for (int i = 0; i < iAnz; i++) SetEvent(hEventRead[i]);
			auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
			duration_sec = (double)duration_ns.count() / (double)1E9;
			t_bytes_per_ns = (double)buffer.length[read_buffer] / duration_ns.count();
			t_bytes_per_second = t_bytes_per_ns * 1E9;
			psTiming->dReadElapsed += duration_sec;
			mcbRead(t_bytes_per_ns);
			if (debug_flags &  D_RUN) printf("READC : D:%lldns\t%6.3fGB/sec\telapsed:%fms  with %6.3fGB/sec\n",
				duration_ns.count(),
				t_bytes_per_ns,
				psTiming->dReadElapsed,
				read_bps_average
				);
			else if (debug_flags &  D_RUN)
				printf("READC : Read %d bytes to Buf%d \t%llx\t%llx\n",
					buffer.length[read_buffer],
					read_buffer,
					newAddress,
					u64FileSize
					);
			newAddress += u32BlockSize;
			read_buffer++;
			if (read_buffer == cBuffers) {
				read_buffer = 0;
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
	int thread_no = *(int *)lpParam;
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
			hEventRead[thread_no], // event handle
			cTimeout);    // indefinite wait
		if (debug_flags &  D_THREAD) printf("WRITE : T%d Got Read Event Write \n", thread_no);
		switch (dwWaitResult) {
			// Event object was signaled
		case WAIT_OBJECT_0:
		{
			if (debug_flags &  D_WRITE) printf("%*sWRITE : T%d\tWriting %d bytes from buf%d to %0llx\n", (thread_no + 1) * 10, " ", thread_no, buffer.length[write_buffer], write_buffer, newAddress);

			auto start = std::chrono::high_resolution_clock::now();
			rc = WriteFile(hFiles[thread_no], buffer.bytes[write_buffer], buffer.length[write_buffer], NULL, &stOverlapped);
			if (!rc)
			{
				switch (dwError = GetLastError()) {
				case  ERROR_IO_PENDING:
				{
					dwWaitResult = WaitForSingleObject(stOverlapped.hEvent, INFINITE);
					auto end = std::chrono::high_resolution_clock::now();
					switch (dwWaitResult) {
					case  WAIT_OBJECT_0:
						rc = GetOverlappedResult(hFiles[thread_no], &stOverlapped, &dwBytesWritten, TRUE);
						printf("WRITE : rc=%0x %0x\n", rc , dwBytesWritten);
						if (rc) {
							assert(dwBytesWritten == buffer.length[write_buffer]);
							if (debug_flags &  D_WRITE) printf("%*sWRITE : T%d\tEvent written\n", (thread_no + 1) * 10, " ", thread_no);
							SetEvent(hEventWritten[thread_no]);
							auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
							duration_sec = (double)duration_ns.count() / (double)1E9;
							t_bytes_per_ns = (double)buffer.length[write_buffer] / duration_ns.count();
							t_bytes_per_second = t_bytes_per_ns  * 1E9;
							psTiming->dWriteElapsed[thread_no] += duration_sec;
							newAddress += dwBytesWritten;
							stOverlapped.OffsetHigh = (newAddress >> 32) & 0xFFFFFFFF;
							stOverlapped.Offset = newAddress & 0xFFFFFFFF;
							write_bps_average = (double)stOverlapped.Offset / psTiming->dWriteElapsed[thread_no] / 1E9;

							max_duration = max(max_duration, duration_sec);
							if (debug_flags &  D_RUN) printf("WRITE : D:%lldns\t%6.3fGB/sec\tE:%fms  with %6.3fGB/sec,%llx\n",
								duration_ns.count(),
								t_bytes_per_ns,
								psTiming->dWriteElapsed[thread_no],
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
								if (debug_flags &  D_WRITE) printf("WRITE : T%d finisch Writing\n",thread_no);
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
								printf("WRITE : T%d Default , ERROR !\n",thread_no);
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
	buffer.bytes[write_buffer][0] = 'E';
	buffer.bytes[write_buffer][1] = 'N';
	buffer.bytes[write_buffer][2] = 'D';
	buffer.bytes[write_buffer][3] = '\0';
	rc = WriteFile(hFiles[thread_no], buffer.bytes[write_buffer], 512, NULL, &stOverlapped);
	if (!rc) {
		switch (GetLastError()) {
		case  ERROR_IO_PENDING:
			dwWaitResult = WaitForSingleObject(stOverlapped.hEvent, INFINITE);
			switch (dwWaitResult) {
			case  WAIT_OBJECT_0:
				rc = GetOverlappedResult(hFiles[thread_no], &stOverlapped, &dwBytesWritten, TRUE);
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

			rc = GetOverlappedResult(hFiles[thread_no], &stOverlapped, &dwBytesWritten, TRUE);
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
	splitData();
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
	buffer.bytes[0] = (UCHAR*)malloc(u32BlockSize);
	ULONG32 value = 1;
	char input[1];

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
		rc = ReadFile(hFile, buffer.bytes[0], u32BlockSize, &dwBytesRead, NULL);
		if (rc) {
			for (UINT32 block = 0; block < dwBytesRead; block += 512) {
				UINT32 v = bNegated ? ~value : value;
				if (bOverwrite) v = 0xFFFFFFFF;
				ilErrors.clear();
				for (UINT32 index = 0; index < 512; index += 4) {
					UINT32 act_value = (buffer.bytes[0][block + index] << 24) |
						(buffer.bytes[0][block + index + 1] << 16) |
						(buffer.bytes[0][block + index + 2] << 8) |
						buffer.bytes[0][block + index + 3]
						;
					if (act_value != v) {
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

	rc = ReadFile(hFile, buffer.bytes[0], 512, &dwBytesRead, NULL);
	if (rc) {
		int rc = strncmp((const char *)buffer.bytes[0], "END", 4);
		if (rc != 0) {
			wprintf(L"VERIFIER : File END of %s is not O.K.\n", filename);
			printf("VERIFIER : Read %02x %02x %02x %02x\n",
				buffer.bytes[0][0],
				buffer.bytes[0][1],
				buffer.bytes[0][2],
				buffer.bytes[0][3]
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

	free(buffer.bytes[0]);
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