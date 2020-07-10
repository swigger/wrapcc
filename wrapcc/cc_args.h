#pragma once
#include <assert.h>

enum eCompileType
{
	CTP_UNKNOWN = 0,
	CTP_C,
	CTP_CPP,
	CTP_IMAGE,
	CTP_LINK = 0x100
};

class cc_args_group
{
public:
	enum
	{
		TYPE_INVALID = 0,
		TYPE_CCFLAGS = 1,
		TYPE_LINKFLAGS = 0x10000,

		TYPE_OTHER = 0x10,
		TYPE_UNKNOWN,
		TYPE_SRC_FILE,
		TYPE_FILE,
	};
	
	int type, idx;
	vector<string> args;
	
public:
	int init(const char * a0);

private:
	int in_check(const char ** grp, const char * fnd, int type, int n);
};

int parse_args(vector<cc_args_group> & ags, vector<cc_args_group> & files, int argc, const char * const * argv);
