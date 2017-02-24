#include "backdoor.h"
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <Windows.h>
#include "Nv.h"

namespace BACKDOOR
{

	struct Header
	{
		int	mCount;
		int mType; 
	};

#define MAX_BACKDOOR_PACKET 1024 	// maximum size of a send/receive packet
#define HEADER_SIZE (sizeof(Header))
#define DATA_SIZE (MAX_BACKDOOR_PACKET - HEADER_SIZE)
#define STRING_SIZE (DATA_SIZE - 1)
#define MAX_ARGS 32
#pragma warning(disable:4996)

	// Chagne this helper class to use the memory mapped IO API relevant to your system.
	// This implementation uses the Windows API for a memory mapped shared file system.
	class MemoryMappedFile
	{
	public:

		MemoryMappedFile(const char *mappingObject,unsigned int mapSize);
		~MemoryMappedFile(void);
		void * getBaseAddress(void);
	private:
		class	MemoryMappedFileImpl	*mImpl;
	};


	class MemoryMappedFileImpl
	{
	public:
		HANDLE	mMapFile;
		void *  mHeader;
	};

	MemoryMappedFile::MemoryMappedFile(const char *mappingObject,unsigned int mapSize)
	{
		mImpl = (MemoryMappedFileImpl *)::malloc(sizeof(MemoryMappedFileImpl));
		mImpl->mHeader = NV_NULL;
		mImpl->mMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,mappingObject);
		if ( mImpl->mMapFile == NV_NULL )
		{
			mImpl->mMapFile = CreateFileMappingA(
				INVALID_HANDLE_VALUE,    // use paging file
				NV_NULL,                    // default security
				PAGE_READWRITE,          // read/write access
				0,                       // maximum object size (high-order DWORD)
				mapSize,                // maximum object size (low-order DWORD)
				mappingObject);
		}
		if ( mImpl->mMapFile )
		{
			mImpl->mHeader = MapViewOfFile(mImpl->mMapFile,FILE_MAP_ALL_ACCESS,0,0,mapSize);
		}

	}

	MemoryMappedFile::~MemoryMappedFile(void)
	{

		if ( mImpl->mHeader )
		{
			UnmapViewOfFile(mImpl->mHeader);
			if ( mImpl->mMapFile )
			{
				CloseHandle(mImpl->mMapFile);
			}
		}

		::free(mImpl);
	}

	void * MemoryMappedFile::getBaseAddress(void)
	{
		return mImpl->mHeader;
	}



class _Backdoor : public Backdoor
{
	MemoryMappedFile	*mReceiveFile;
	MemoryMappedFile	*mSendFile;
	const char			*mArgv[MAX_ARGS];
	char				mBuffer[MAX_BACKDOOR_PACKET];

	int	mSendCount;
	int	mReceiveCount;

public:
	_Backdoor(const char *sendName,	// The name of the shared memory file to act as the 'sender' stream.
 			  const char *receiveName) // The name of the shared memory file to act as the 'receive' message stream.
	{
		mSendCount = 0;	// This is a semaphore used to signal when a new message has been sent.
		mReceiveCount = 0; // This is a semaphore used to detect if a new message has been received.
		mReceiveFile = new MemoryMappedFile(receiveName,MAX_BACKDOOR_PACKET);	// Open the receive stream file
		mSendFile = new MemoryMappedFile(sendName,MAX_BACKDOOR_PACKET); // Open the send stream file
	}

	virtual ~_Backdoor(void)
	{
		delete mReceiveFile; // Close the receive stream file
		delete mSendFile;	// Close the send stream file.
	}

	virtual void send(const char *fmt,...)
	{
		char wbuff[DATA_SIZE];
		wbuff[STRING_SIZE] = 0;
		_vsnprintf(wbuff,STRING_SIZE, fmt, (char *)(&fmt+1));

		mSendCount++;	// Increment the send counter.
		size_t len = strlen(wbuff);

		if ( len < STRING_SIZE && mSendFile )
		{
			char *baseAddress = (char *)mSendFile->getBaseAddress();	// get the base address of the shared memory
			int *dest = (int *)baseAddress;

			baseAddress+=sizeof(Header);

			memcpy(baseAddress,wbuff,len+1);	// First copy the message.
			*dest = mSendCount;				// Now revised the send count semaphore so the other process knows there is a new message to processs.
		}
	}

	virtual const char **getInput(int &argc)
	{
		const char **ret = NV_NULL;
		argc = 0;

		if ( mReceiveFile )
		{
			const char *baseAddress = (const char *)mReceiveFile->getBaseAddress();
			const int *source = (const int *)baseAddress;
			baseAddress+=sizeof(Header);
			if ( *source != mReceiveCount )
			{
				mReceiveCount = *source;
				memcpy(mBuffer,baseAddress,DATA_SIZE);

				char *scan = mBuffer;
				while ( *scan == 32 ) scan++;	// skip leading spaces
				if ( *scan )	// if not EOS
				{
					argc = 1;
					mArgv[0] = scan;	// set the first argument

					while ( *scan && argc < MAX_ARGS)	// while still data and we haven't exceeded the maximum argument count.
					{
						while ( *scan && *scan != 32 ) scan++;	// scan forward to the next space
						if ( *scan == 32 )	// if we hit a space
						{
							*scan = 0; // null-terminate the argument
							scan++;	// skip to the next character
							while ( *scan == 32 ) scan++;	// skip any leading spaces
							if ( *scan )	// if there is still a valid non-space character process that as the next argument
							{
								mArgv[argc] = scan;
								argc++;
							}
						}
					}
					ret = mArgv;
				}
			}
		}

		return ret;
	}

	bool sendData(char* ptr, size_t size, int dataType)
	{
		if ( size > DATA_SIZE || !mSendFile )
			return false;

		mSendCount++;	// Increment the send counter.

		char *baseAddress = (char *)mSendFile->getBaseAddress();	// get the base address of the shared memory

		Header* header = (Header*)baseAddress;

		header->mCount = mSendCount;
		header->mType = dataType;

		baseAddress+=sizeof(Header);
		memcpy(baseAddress,ptr, size);

		return true;
	}

	bool receiveData(char* ptr, size_t size, int dataType)
	{
		if ( size > DATA_SIZE || !mReceiveFile )
			return false;

		const char *baseAddress = (const char *)mReceiveFile->getBaseAddress();
		const int *source = (const int *)baseAddress;
		
		Header* header = (Header*)baseAddress;
		if (header->mType != dataType)
			return false;

		baseAddress+=sizeof(Header);		

		if ( *source != mReceiveCount )
		{
			mReceiveCount = *source;
			memcpy(ptr, baseAddress, size);
		}

		return true;
	}

	bool checkMessage(const char* msg)
	{
		if (!mReceiveFile)
			return false;

		const char *baseAddress = (const char *)mReceiveFile->getBaseAddress();
		const int *source = (const int *)baseAddress;

		if (*source == mReceiveCount)
			return false;

		if ( *source != mReceiveCount )
		{
			baseAddress+=sizeof(Header);

			return (!strcmp(baseAddress, msg));
		}

		return false;
	}

	virtual void release(void)
	{
		delete this;
	}


};

}; // end of namespace

using namespace BACKDOOR;


Backdoor * createBackdoor(const char *sendName,const char *receiveName)
{
	_Backdoor *ret = new _Backdoor(sendName,receiveName);
	return static_cast< Backdoor *>(ret);
}

