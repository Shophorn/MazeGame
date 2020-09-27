internal void read_settings_file(SkySettings & skySettings, Tree3Settings & tree0, Tree3Settings & tree1)
{
	PlatformFileHandle file = platform_file_open("settings", FILE_MODE_READ);

	s32 fileSize 	= platform_file_get_size(file);
	char * buffer 	= push_memory<char>(*global_transientMemory, fileSize, 0);

	platform_file_read(file, fileSize, buffer);
	platform_file_close(file);	

	String fileAsString = {fileSize, buffer};
	String originalString = fileAsString;

	while(fileAsString.length > 0)
	{
		String line 		= string_extract_line (fileAsString);
		String identifier 	= string_extract_until_character(line, '=');

		string_extract_until_character(fileAsString, '[');
		string_extract_line(fileAsString);

		String block = string_extract_until_character(fileAsString, ']');

		auto deserialize_if_id_matches = [&identifier, &block](char const * name, auto & serializedProperties)
		{
			if(string_equals(identifier, name))
			{
				deserialize_properties(serializedProperties, block);
			}
		};

		deserialize_if_id_matches("sky", skySettings);
		deserialize_if_id_matches("tree_0", tree0);
		deserialize_if_id_matches("tree_1", tree1);

		string_eat_leading_spaces_and_lines(fileAsString);
	}
}

internal void write_settings_file(	SkySettings const & skySettings,
									Tree3Settings const & tree0,
									Tree3Settings const & tree1)
{
	PlatformFileHandle file = platform_file_open("settings", FILE_MODE_WRITE);

	auto serialize = [file](char const * label, auto const & serializedData)
	{
		push_memory_checkpoint(*global_transientMemory);
	
		constexpr s32 capacity 			= 2000;
		String serializedFormatString 	= push_temp_string(capacity);

		string_append_format(serializedFormatString, capacity, label, " = \n[\n");
		serialize_properties(serializedData, serializedFormatString, capacity);
		string_append_cstring(serializedFormatString, capacity, "]\n\n");

		platform_file_write(file, serializedFormatString.length, serializedFormatString.memory);

		pop_memory_checkpoint(*global_transientMemory);
	};

	serialize("sky", skySettings);
	serialize("tree_0", tree0);
	serialize("tree_1", tree1);

	platform_file_close(file);
}
