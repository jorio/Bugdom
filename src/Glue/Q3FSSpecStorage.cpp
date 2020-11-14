#include "Pomme.h"
#include <QuesaStorage.h>  // For TQ3StorageObject
//#include "QD3DStorage.h"

extern "C"
{
	TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec);
}

//-----------------------------------------------------------------------------
// Pomme extensions for Quesa

TQ3StorageObject Q3FSSpecStorage_New(const FSSpec* spec)
{
	short refNum;
	long fileLength;
	
	if (noErr != FSpOpenDF(spec, fsRdPerm, &refNum))
		throw std::runtime_error("Q3FSSpecStorage_New: couldn't open file");
	
	GetEOF(refNum, &fileLength);
	Ptr buffer = NewPtr(fileLength);
	FSRead(refNum, &fileLength, buffer);
	FSClose(refNum);
	
	TQ3StorageObject storageObject = Q3MemoryStorage_New(reinterpret_cast<const unsigned char*>(buffer), fileLength);

	// Q3MemoryStorage_New creates a copy of the buffer, so we don't need to keep it around
	DisposePtr(buffer);

	return storageObject;
}
