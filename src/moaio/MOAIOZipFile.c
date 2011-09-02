// Copyright (c) 2010-2011 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"
#include <moaio\moaio_util.h>
#include <moaio\MOAIOZipFile.h>

#define SCAN_BUFFER_SIZE 256

#define ARCHIVE_HEADER_SIGNATURE  0x06054b50
#define ENTRY_HEADER_SIGNATURE  0x02014b50
#define FILE_HEADER_SIGNATURE  0x04034b50

//================================================================//
// ZIPArchiveHeader
//================================================================//
typedef struct ZIPArchiveHeader {

	unsigned long	mSignature;			// 4 End of central directory signature = 0x06054b50
	unsigned short	mDiskNumber;		// 2 Number of this disk
	unsigned short	mStartDisk;			// 2 Disk where central directory starts
	unsigned short	mTotalDiskEntries;	// 2 Total number of entries on disk
	unsigned short	mTotalEntries;		// 2 Total number of central in archive
	unsigned long	mCDSize;			// 4 Size of central directory in bytes
	unsigned long	mCDAddr;			// 4 Offset of start of central directory, relative to start of archive
	unsigned short	mCommentLength;		// 2 ZIP file comment length

} ZIPArchiveHeader;

//================================================================//
// ZIPEntryHeader
//================================================================//
typedef struct ZIPEntryHeader {

	unsigned long	mSignature;				// 4 Central directory file header signature = 0x02014b50
	unsigned short	mByVersion;				// 2 Version made by
	unsigned short	mVersionNeeded;			// 2 Version needed to extract (minimum)
	unsigned short	mFlag;					// 2 General purpose bit flag
	unsigned short	mCompression;			// 2 Compression method
	unsigned short	mLastModTime;			// 2 File last modification time
	unsigned short	mLastModDate;			// 2 File last modification date
	unsigned long	mCrc32;					// 4 CRC-32
	unsigned long	mCompressedSize;		// 4 Compressed size
	unsigned long	mUncompressedSize;		// 4 Uncompressed size
	unsigned short	mNameLength;			// 2 File name length (n)
	unsigned short	mExtraFieldLength;		// 2 Extra field length (m)
	unsigned short	mCommentLength;			// 2 File comment length (k)
	unsigned short	mDiskNumber;			// 2 Disk number where file starts
	unsigned short	mInternalAttributes;	// 2 Internal file attributes
	unsigned long	mExternalAttributes;	// 4 External file attributes
	unsigned long	mFileHeaderAddr;		// 4 Relative offset of file header

} ZIPEntryHeader;

//================================================================//
// ZIPFileHeader
//================================================================//
typedef struct ZIPFileHeader {

	unsigned long	mSignature;				// 4	Local file header signature = 0x04034b50 (read as a little-endian number)
	unsigned short	mVersionNeeded;			// 2	Version needed to extract (minimum)
	unsigned short	mFlag;					// 2	General purpose bit flag
	unsigned short	mCompression;			// 2	Compression method
	unsigned short	mLastModTime;			// 2	File last modification time
	unsigned short	mLastModDate;			// 2	File last modification date
	unsigned long	mCrc32;					// 4	CRC-32
	unsigned long	mCompressedSize;		// 4	Compressed size
	unsigned long	mUncompressedSize;		// 4	Uncompressed size
	unsigned short	mNameLength;			// 2	File name length
	unsigned short	mExtraFieldLength;		// 2	Extra field length

} ZIPFileHeader;

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
static int read_archive_header ( FILE* file, ZIPArchiveHeader* header ) {

	size_t filelen;
	size_t cursor;
	char buffer [ SCAN_BUFFER_SIZE ];
	size_t stepsize;
	size_t scansize;
	int i;
	
	if ( !file ) return -1;

	fseek ( file, 0, SEEK_END );
	filelen = ftell ( file );
	
	cursor = filelen;
	stepsize = SCAN_BUFFER_SIZE - 4;
    while ( cursor ) {
		
		cursor = ( cursor > stepsize ) ? cursor - stepsize : 0;
		scansize = (( cursor + SCAN_BUFFER_SIZE ) > filelen ) ? filelen - cursor : SCAN_BUFFER_SIZE;
		
		fseek ( file, cursor, SEEK_SET );
		fread ( buffer, scansize, 1, file );

        for ( i = scansize - 4; i > 0; --i ) {
			
			// maybe found it
			if ( *( unsigned long* )&buffer [ i ] == ARCHIVE_HEADER_SIGNATURE ) {

				fseek ( file, cursor + i, SEEK_SET );
				
				fread ( &header->mSignature, 4, 1, file );
				fread ( &header->mDiskNumber, 2, 1, file );
				fread ( &header->mStartDisk, 2, 1, file );
				fread ( &header->mTotalDiskEntries, 2, 1, file );
				fread ( &header->mTotalEntries, 2, 1, file );
				fread ( &header->mCDSize, 4, 1, file );
				fread ( &header->mCDAddr, 4, 1, file );
				fread ( &header->mCommentLength, 2, 1, file );
				
				return 0;
			}
		}
    }
	return -1;
}

//----------------------------------------------------------------//
static int read_entry_header ( FILE* file, ZIPEntryHeader* header ) {
	
	fread ( &header->mSignature, 4, 1, file );
	
	if ( header->mSignature != ENTRY_HEADER_SIGNATURE ) return -1;
	
	fread ( &header->mByVersion, 2, 1, file );
	fread ( &header->mVersionNeeded, 2, 1, file );
	fread ( &header->mFlag, 2, 1, file );
	fread ( &header->mCompression, 2, 1, file );
	fread ( &header->mLastModTime, 2, 1, file );
	fread ( &header->mLastModDate, 2, 1, file );
	fread ( &header->mCrc32, 4, 1, file );
	fread ( &header->mCompressedSize, 4, 1, file );
	fread ( &header->mUncompressedSize, 4, 1, file );
	fread ( &header->mNameLength, 2, 1, file );
	fread ( &header->mExtraFieldLength, 2, 1, file );
	fread ( &header->mCommentLength, 2, 1, file );
	fread ( &header->mDiskNumber, 2, 1, file );
	fread ( &header->mInternalAttributes, 2, 1, file );
	fread ( &header->mExternalAttributes, 4, 1, file );
	fread ( &header->mFileHeaderAddr, 4, 1, file );
	
	return 0;
}

//----------------------------------------------------------------//
static int read_file_header ( FILE* file, ZIPFileHeader* header ) {
	
	fread ( &header->mSignature, 4, 1, file );
	
	if ( header->mSignature != FILE_HEADER_SIGNATURE ) return -1;
	
	fread ( &header->mVersionNeeded, 2, 1, file );
	fread ( &header->mFlag, 2, 1, file );
	fread ( &header->mCompression, 2, 1, file );
	fread ( &header->mLastModTime, 2, 1, file );
	fread ( &header->mLastModDate, 2, 1, file );
	fread ( &header->mCrc32, 4, 1, file );
	fread ( &header->mCompressedSize, 4, 1, file );
	fread ( &header->mUncompressedSize, 4, 1, file );
	fread ( &header->mNameLength, 2, 1, file );
	fread ( &header->mExtraFieldLength, 2, 1, file );
	
	return 0;
}

//================================================================//
// MOAIOZipFile
//================================================================//

//----------------------------------------------------------------//
void MOAIOZipFile_Delete ( MOAIOZipFile* self ) {

	int i;
	MOAIOZipEntry* entry;

	if ( !self ) return;
	
	clear_string ( self->mFilename );
	
	if ( self->mEntries ) {
		for ( i = 0; i < self->mTotalEntries; ++i ) {
			entry = &self->mEntries [ i ];
			clear_string ( entry->mName );
		}
		free ( self->mEntries );
	}
	free ( self );
}

//----------------------------------------------------------------//
MOAIOZipEntry* MOAIOZipFile_FindEntry ( MOAIOZipFile* self, char const* name ) {

	int i;
	
	for ( i = 0; i < self->mTotalEntries; ++i ) {
		MOAIOZipEntry* entry = &self->mEntries [ i ];
		if ( strcmp ( name, entry->mName ) == 0 ) return entry;
	}
	return 0;
}

//----------------------------------------------------------------//
MOAIOZipFile* MOAIOZipFile_New ( const char* filename ) {

	MOAIOZipFile* self = 0;
	ZIPArchiveHeader header;
	ZIPEntryHeader entryHeader;
	MOAIOZipEntry* entry;
	int result;
	int i;

	FILE* file = fopen ( filename, "rb" );
	if ( !file ) goto error;
 
	result = read_archive_header ( file, &header );
	if ( result ) goto error;
	
	if ( header.mDiskNumber != 0 ) goto error; // unsupported
	if ( header.mStartDisk != 0 ) goto error; // unsupported
	if ( header.mTotalDiskEntries != header.mTotalEntries ) goto error; // unsupported
	
	// seek to top of central directory
	fseek ( file, header.mCDAddr, SEEK_SET );
	
	// create the info
	self = ( MOAIOZipFile* )calloc ( 1, sizeof ( MOAIOZipFile ));
	
	self->mFilename = copy_string ( filename );
	self->mTotalEntries = header.mTotalEntries;
	self->mEntries = ( MOAIOZipEntry* )malloc ( sizeof ( MOAIOZipEntry ) * header.mTotalEntries );
	
	// parse in the entries
	for ( i = 0; i < header.mTotalEntries; ++i ) {
	
		result = read_entry_header ( file, &entryHeader );
		if ( result ) goto error;
		
		entry = &self->mEntries [ i ];
		
		entry->mFileHeaderAddr		= entryHeader.mFileHeaderAddr;
		entry->mCrc32				= entryHeader.mCrc32;
		entry->mCompression			= entryHeader.mCompression;
		entry->mCompressedSize		= entryHeader.mCompressedSize;
		entry->mUncompressedSize	= entryHeader.mUncompressedSize;
		
		entry->mName = ( char* )malloc ( entryHeader.mNameLength + 1 );
		entry->mName [ entryHeader.mNameLength ] = 0;
		
		fread ( entry->mName, entryHeader.mNameLength, 1, file );
		fseek ( file, entryHeader.mCommentLength + entryHeader.mExtraFieldLength, SEEK_CUR );
	}
	
	if ( file ) {
		fclose ( file );
	}
	
	return self;

error:

	if ( file ) {
		fclose ( file );
	}

	if ( self ) {
		MOAIOZipFile_Delete ( self );
	}
	return 0;
}

//================================================================//
// MOAIOZipStream
//================================================================//

//----------------------------------------------------------------//
int MOAIOZipStream_Close ( MOAIOZipStream* self ) {

	if ( !self ) return 0;

	if ( self->mFile ) {
		fclose ( self->mFile );
		inflateEnd ( &self->mStream );
	}
	
	if ( self->mBuffer ) {
		free ( self->mBuffer );
	}
	return 0;
}

//----------------------------------------------------------------//
MOAIOZipStream* MOAIOZipStream_Open ( MOAIOZipFile* archive, const char* entryname ) {

	int result;
	FILE* file = 0;
	MOAIOZipStream* self = 0;
	MOAIOZipEntry* entry;
	ZIPFileHeader fileHeader;
	
	entry = MOAIOZipFile_FindEntry ( archive, entryname );
	if ( !entry ) goto error;

	file = fopen ( archive->mFilename, "rb" );
	if ( !file ) goto error;

    self = ( MOAIOZipStream* )calloc ( 1, sizeof ( MOAIOZipStream ));

	self->mFile = file;
	self->mEntry = entry;
	// finfo->entry = (( entry->symlink != NULL ) ? entry->symlink : entry );

	self->mBufferSize = entry->mCompressedSize < ZIP_STREAM_BUFFER_MAX ? entry->mCompressedSize : ZIP_STREAM_BUFFER_MAX;
	self->mBuffer = malloc ( self->mBufferSize );

	result = fseek ( file, entry->mFileHeaderAddr, SEEK_SET );
	if ( result ) goto error;

	// read local header
	result = read_file_header ( file, &fileHeader );
	if ( result ) goto error;
	
	// sanity check
	if ( fileHeader.mCrc32 != entry->mCrc32 ) goto error;

	result = fseek ( file, fileHeader.mNameLength + fileHeader.mExtraFieldLength, SEEK_CUR );
	if ( result ) goto error;

	self->mBaseAddr = ftell ( file );

    if ( entry->mCompression ) {
		result = inflateInit2 ( &self->mStream, -MAX_WBITS );
		if ( result != Z_OK ) goto error;
    }
	return self;

error:

	if ( self ) {
		MOAIOZipStream_Close ( self );
	}
	return 0;
}

//----------------------------------------------------------------//
size_t MOAIOZipStream_Read ( MOAIOZipStream* self, void* buffer, size_t size ) {

	int result;
	FILE* file = self->mFile;
	z_stream* stream = &self->mStream;
    MOAIOZipEntry* entry = self->mEntry;
    size_t totalRead = 0;
    size_t totalOut = 0;
    size_t remaining = entry->mUncompressedSize - self->mUncompressedCursor;

	if ( !entry->mCompression ) {
		return fread ( buffer, size, 1, file );
	}

	if ( !file ) return 0;
	if ( !stream ) return 0;
	if ( !size ) return 0;
	if ( !remaining ) return 0;

	if ( remaining < size ) {
		size = remaining;
    }
    
	stream->next_out = buffer;
	stream->avail_out = size;

    while ( totalRead < size ) {
		
		if ( stream->avail_in == 0 ) {
			
			size_t cacheSize = entry->mCompressedSize - self->mCompressedCursor;
			
			if ( cacheSize > 0 ) {
				if ( cacheSize > self->mBufferSize ) {
					cacheSize = self->mBufferSize;
				}

				cacheSize = fread ( self->mBuffer, 1, cacheSize, file );
				if ( cacheSize <= 0 ) break;

				self->mCompressedCursor += cacheSize;
				stream->next_in = ( Bytef* )self->mBuffer;
				stream->avail_in = cacheSize;
			}
		}

		totalOut = stream->total_out;
		result = inflate ( stream, Z_SYNC_FLUSH );
		totalRead += ( stream->total_out - totalOut );
		
		if ( result != Z_OK ) break;
	}

	if ( totalRead > 0 ) {
		self->mUncompressedCursor += totalRead;
	}

    return totalRead;
}

//----------------------------------------------------------------//
int MOAIOZipStream_Seek ( MOAIOZipStream* self, long int offset, int origin ) {

	int result;
	size_t absOffset = 0;
	FILE* file = self->mFile;
	z_stream* stream = &self->mStream;
    MOAIOZipEntry* entry = self->mEntry;

	switch ( origin ) {
		case SEEK_CUR: {
			absOffset = self->mUncompressedCursor + offset;
			break;
		}
		case SEEK_END: {
			absOffset = entry->mUncompressedSize;
			break;
		}
		case SEEK_SET: {
			absOffset = offset;
			break;
		}
	}

	if ( absOffset > entry->mUncompressedSize ) return -1;

	if ( !entry->mCompression ) {
		return fseek ( file, offset, SEEK_SET );
	}

    if ( absOffset < self->mUncompressedCursor ) {
		
		z_stream newStream;
		memset ( &newStream, 0, sizeof ( z_stream ));
		
		result = fseek ( file, self->mBaseAddr, SEEK_SET );
		if ( result ) return -1;
		
		result = inflateInit2 ( &newStream, -MAX_WBITS );
		if ( result != Z_OK ) return -1;

		inflateEnd ( stream );
		( *stream ) = newStream;

		self->mCompressedCursor = 0;
		self->mUncompressedCursor = 0;
    }

    while ( self->mUncompressedCursor < absOffset ) {
    
		char buffer [ SCAN_BUFFER_SIZE ];
		size_t size;
		size_t read;

		size = offset - self->mUncompressedCursor;
		if ( size > SCAN_BUFFER_SIZE ) {
			size = SCAN_BUFFER_SIZE;
		}
		
		read = MOAIOZipStream_Read ( self, buffer, size );
		if ( read != size ) return -1;
	}

    return 0;
}

//----------------------------------------------------------------//
size_t MOAIOZipStream_Tell ( MOAIOZipStream* self ) {

	return self->mUncompressedCursor;
}
