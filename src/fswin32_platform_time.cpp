/*
Leo Tamminen

Time api implementation
*/
s64 platform_time_now () 
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return t.QuadPart;
}

f64 platform_time_elapsed_seconds(s64 start, s64 end)
{
	local_persist s64 frequency = []()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		
		// Todo(Leo): actually append bigger values also
		// log_console(0, "performance frequency = ", (s32)frequency.QuadPart);

		return frequency.QuadPart;
	}();

	f64 seconds = static_cast<f64>(end - start) / frequency;
	return seconds;
}