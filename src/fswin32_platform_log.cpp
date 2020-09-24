/*
Leo Tamminen

What you gonna do, this function now has its own file.
*/

void platform_log_write(s32 count, char const * buffer)
{
	local_persist HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD bytesWritten 	= 0;
	BOOL success 		= WriteConsole(console, buffer, count, &bytesWritten, nullptr);
	
	// Todo(Leo): if we fail to log, what can we do, but this can be uncommented for debugger purposes
	// DWORD error 		= 0;
	// if(success == false)
	// {
	// 	error = GetLastError();
	// };

	// Todo(Leo): this should not be necessary, but lest I forget its existance, I leave it here
	// FlushFileBuffers(context->console);
}