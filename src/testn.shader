	#define THING(name) if (string_equals(id, #name)) {return tree.name;})

	THING(maxHeight);

	#undef THING