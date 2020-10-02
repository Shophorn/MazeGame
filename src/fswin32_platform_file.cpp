/*
Leo Tamminen

platform_file_XXXX functions' implementations.
*/

PlatformFileHandle platform_file_open(char const * filename, FileMode fileMode)
{
	DWORD access;
	// Todo(Leo): this should only be enabled in development
	DWORD share = 0;
	if(fileMode == FILE_MODE_READ)
	{
		access = GENERIC_READ;
		share = FILE_SHARE_READ;
	}
	else if (fileMode == FILE_MODE_WRITE)
	{
		access = GENERIC_WRITE;
	}

	// Todo(Leo): check lpSecurityAttributes
	HANDLE file = CreateFile(	filename,
								access,
								share,
								nullptr,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								nullptr);

	DWORD error = 0;
	if(file == INVALID_HANDLE_VALUE)
	{
		error = GetLastError();
	}


	SetFilePointer((HANDLE)file, 0, nullptr, FILE_BEGIN);

	// Todo(Leo): this may be unwanted
	if (fileMode == FILE_MODE_WRITE)
	{
		SetEndOfFile((HANDLE)file);
	}

	PlatformFileHandle result = (PlatformFileHandle)file;
	return result;
}

void platform_file_close(PlatformFileHandle file)
{
	CloseHandle((HANDLE)file);
}

void platform_file_set_position(PlatformFileHandle file, s64 location)
{
	LARGE_INTEGER position;
	position.QuadPart = location;
	SetFilePointer((HANDLE)file, position.LowPart, &position.HighPart, FILE_BEGIN);
}

s64 platform_file_get_position(PlatformFileHandle file)
{
	LARGE_INTEGER position;
	position.LowPart = SetFilePointer((HANDLE)file, 0, &position.HighPart, FILE_CURRENT);
	return position.QuadPart;
}

void platform_file_write (PlatformFileHandle file, s64 count, void * memory)
{
	DWORD bytesWritten;
	WriteFile((HANDLE)file, memory, count, &bytesWritten, nullptr);
}

void platform_file_read (PlatformFileHandle file, s64 count, void * memory)
{
	// Todo(Leo): test if ReadFile also moves file pointer, WriteFile does
	DWORD bytesRead;
	ReadFile((HANDLE)file, memory, count, &bytesRead, nullptr);

	SetFilePointer((HANDLE)file, count, nullptr, FILE_CURRENT);
}

s64 platform_file_get_size(PlatformFileHandle file)
{
	LARGE_INTEGER fileSize;
	// GetFileSizeEx((HANDLE)file, &fileSize);

	DWORD highPart = 0;
	DWORD lowPart = GetFileSize((HANDLE)file, &highPart);
	DWORD error;
	if (lowPart == 0xffffffff)
	{
		error = GetLastError();
	}

	fileSize.LowPart = lowPart; 
	fileSize.HighPart = highPart;

	Assert(fileSize.QuadPart < max_value_s64);

	return fileSize.QuadPart;
}

// ----------- These do not belong to platform api ------------

internal FILETIME fswin32_file_get_write_time(const char * fileName)
{
    // Todo(Leo): Make this function sensible
    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    if (GetFileAttributesExA(fileName, GetFileExInfoStandard, &fileInfo))
    {
    }
    else
    {
        // Todo(Leo): Now what??? Getting file time failed --> file does not exist??
    }    
    FILETIME result = fileInfo.ftLastWriteTime;
    return result;
}