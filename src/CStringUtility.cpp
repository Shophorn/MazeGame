internal s32
compare_cstring(char const * lhs, char const * rhs)
{
	while(*lhs != 0 and *rhs != 0 and *lhs == *rhs)
	{
		++lhs;
		++rhs;
	}

	if (*lhs == *rhs)
		return 0;

	if (*lhs < *rhs)
		return -1;

	if (*lhs >  *rhs)
		return 1;


	Assert(false);

	return 0;
}

internal bool
cstring_equals(char const * a, char const * b)
{
	return compare_cstring(a, b) == 0;
}