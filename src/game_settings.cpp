/*
Leo Tamminen

Friendsimulator game specific serialized objects
*/



template<typename ... Ts>
internal void read_settings_file(SerializedPropertyList<Ts...> serializedObjects)
{
	PlatformFileHandle file = platform_file_open("settings", FILE_MODE_READ);

	s32 fileSize 	= platform_file_get_size(file);
	char * buffer 	= push_memory<char>(*global_transientMemory, fileSize, ALLOC_GARBAGE);

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

		for_each_property (
			[&identifier, &block](auto serializedObject)
			{
				if (string_equals(identifier, serializedObject.name))
				{
					deserialize_properties(serializedObject.data, block);
				}
			},
			serializedObjects
		);

		string_eat_leading_spaces_and_lines(fileAsString);
	}
}

template<typename ... Ts>
internal void write_settings_file(PlatformFileHandle file, SerializedPropertyList<Ts...> serializedObjects)
{

	for_each_property (
		[file](auto serializedObject)
		{
			auto checkpoint = memory_push_checkpoint(*global_transientMemory);
		
			// Todo(Leo): Maybe we should put capacity into string itself, hmm, hmm
			constexpr s32 capacity 			= 2000;
			String serializedFormatString 	= push_temp_string(capacity);

			char const * label = serializedObject.name;

			string_append_format(serializedFormatString, capacity, label, " = \n[\n");
			serialize_properties(serializedObject.data, serializedFormatString, capacity);
			string_append_cstring(serializedFormatString, capacity, "]\n\n");

			platform_file_write(file, serializedFormatString.length, serializedFormatString.memory);

			memory_pop_checkpoint(*global_transientMemory, checkpoint);
		},	
		serializedObjects
	);
}
