/*
Leo Tamminen

Time api implementation
*/
PlatformTimePoint platform_time_now () 
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return {t.QuadPart};
}

f64 platform_time_elapsed_seconds(PlatformTimePoint start, PlatformTimePoint end)
{
	local_persist s64 frequency = []()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		
		// Todo(Leo): actually append bigger values also
		// logConsole(0, "performance frequency = ", (s32)frequency.QuadPart);

		return frequency.QuadPart;
	}();

	f64 seconds = (f64)(end.value - start.value) / frequency;
	return seconds;
}