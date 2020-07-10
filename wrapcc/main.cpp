//
//  main.cpp
//  wrapcc
//
//  Created by xungeng on 16/1/29.
//  Copyright © 2016年 xungeng. All rights reserved.
//

#include "stdafx.h"
#include "cc_args.h"
#ifdef __CYGWIN__
#include <cygwin/stdlib.h>
#endif
#ifdef __APPLE__
#include <mach-o/loader.h>
#include <mach-o/dyld.h>
#endif
#include "loadenv.h"
#include "sutil.h"

#ifdef _WIN32
void unsetenv(const char * v)
{
	string s(v);
	s += "=";
	putenv(s.c_str());
}
#endif


static void parse_args(vector<cc_args_group> & ags, vector<cc_args_group> & files, const vector<string> & argvs,
					   int idxbase)
{
	const char ** argv = new const char * [argvs.size()+1];
	argv[argvs.size()] = 0;
	for (size_t i=0; i<argvs.size(); ++i)
		argv[i] = argvs[i].c_str();
	parse_args(ags, files, (int)argvs.size(), argv);
	for (auto & v : ags) v.idx += idxbase;
	for (auto & v : files) v.idx += idxbase;
	delete [] argv;
}

static bool is_arg(const cc_args_group & ag, const char * test)
{
	//test if `test` is a type of ag.
	if (test[2] == 0)
	{
		//single options. ex: -DDEBUG is a type of -D
		return sutil::s_begin_with(ag.args[0].c_str(), test);
	}
	else
	{
		//long options. must be equal or at =
		//ex: -std=gnu99 is a type of -std, however, -stdlib=libc++ is not a type of -std
		const char * a = ag.args[0].c_str();
		size_t tlen = strlen(test);
		if (strncmp(a, test, tlen)!= 0) return false;
		if (a[tlen] == 0 || a[tlen] == '=') return true;
		return false;
	}
}

static void modify(vector<cc_args_group> & ags, vector<cc_args_group>& files, int type)
{
	vector<cc_args_group> files_tmp;
	vector<cc_args_group> ags_cflags, ags_ldflags;

	string cflags_, lflags_;
	if ((type & 0xff) == CTP_CPP)
		cflags_ = safe_env("WRAPCC_CXXFLAGS");
	else if ((type&0xff) == CTP_C)
		cflags_ = safe_env("WRAPCC_CFLAGS");
	if (type & CTP_LINK)
		lflags_ = safe_env("WRAPCC_LDFLAGS");

	vector<string> cflags = sutil::split_str(cflags_, " ");
	vector<string> lflags = sutil::split_str(lflags_, " ");

	sutil::remove_empty(cflags);
	sutil::remove_empty(lflags);
	parse_args(ags_cflags, files_tmp, cflags, -0x10000);
	parse_args(ags_ldflags, files_tmp, lflags, -0x10000);
	if (files_tmp.size() > 0)
	{
		fprintf(stderr, "unexpected files:\n");
		for (auto f : files_tmp)
		{
			fprintf(stderr, " %s\n", f.args[0].c_str());
		}
	}
	assert(files_tmp.size() == 0);

	ags.insert(ags.begin(), ags_cflags.begin(), ags_cflags.end());
	ags.insert(ags.begin(), ags_ldflags.begin(), ags_ldflags.end());

	//now add important post flags.
	ags_cflags.clear();
	ags_ldflags.clear();

	cflags_  = "";
	if ((type & 0xff) == CTP_CPP)
		cflags_ = safe_env("WRAPCC_CXXFLAGS2");
	else if ((type&0xff) == CTP_C)
		cflags_ = safe_env("WRAPCC_CFLAGS2");
	lflags_  = "";
	if (type & CTP_LINK) lflags_ = safe_env("WRAPCC_LDFLAGS2");

	cflags = sutil::split_str(cflags_, " ");
	lflags = sutil::split_str(lflags_, " ");

	sutil::remove_empty(cflags);
	sutil::remove_empty(lflags);
	parse_args(ags_cflags, files_tmp, cflags, 0x10000);
	parse_args(ags_ldflags, files_tmp, lflags, 0x10000);
	assert(files_tmp.size() == 0);

	ags.insert(ags.end(), ags_cflags.begin(), ags_cflags.end());
	ags.insert(ags.end(), ags_ldflags.begin(), ags_ldflags.end());

	//now remove some flags....
	cflags_ = safe_env("WRAPCC_REMOVE");
	cflags = sutil::split_str(cflags_, " ");
	sutil::remove_empty(cflags);
	ags_cflags.clear();
	parse_args(ags_cflags, files_tmp, cflags, 0);
	//assert(files_tmp.size() == 0);

	for (cc_args_group & ag : ags)
	{
		if (ag.type == cc_args_group::TYPE_INVALID) continue;
		for (cc_args_group & ag1 : ags_cflags)
		{
			if (ag.args == ag1.args)
			{
				ag.type = cc_args_group::TYPE_INVALID;
				break;
			}
		}
	}
	for (cc_args_group & ag : files)
	{
		if (ag.type == cc_args_group::TYPE_INVALID) continue;
		for (cc_args_group & ag1 : files_tmp)
		{
			if (ag.args == ag1.args)
			{
				ag.type = cc_args_group::TYPE_INVALID;
				break;
			}
		}
	}

	//now strip incompatible args.
	//这些选项只应该出现一次，以最后一个为准
	const char * in_compatible[] = {
		"-g", "-O", "-x", "-arch", "-isysroot", "-std", "-stdlib", "-miphoneos-version-min",
		0
	};

	for (int i=0; in_compatible[i]; ++i)
	{
		const char * ic = in_compatible[i];
		vector<size_t> indecies;
		for (size_t i=0; i<ags.size(); ++i)
		{
			if (ags[i].type == cc_args_group::TYPE_INVALID)
				continue;

			if (is_arg(ags[i], ic))
				indecies.push_back(i);
		}
		for (size_t i=0; i+1<indecies.size(); ++i)
		{
			ags[indecies[i]].type = cc_args_group::TYPE_INVALID;
		}
	}
}

static void append_cfg(string & cfgfile)
{
	if (cfgfile.length() >= 4 && strcasecmp(cfgfile.c_str() + cfgfile.length() - 4, ".exe") == 0)
	{
		cfgfile = cfgfile.substr(0, cfgfile.length() - 4) + ".wrapcc.cfg";
	}
	else
		cfgfile += ".wrapcc.cfg";
}

static string find_configfile()
{
	struct stat st;
	string cfgfile;
#ifdef __GNUC__
	if (stat("/proc/self/exe", &st) == 0)
	{
		cfgfile = cann_name("/proc/self/exe");
		append_cfg(cfgfile);
		if (stat(cfgfile.c_str(), &st) == 0)
			return cfgfile;
	}
#endif

#ifdef __APPLE__
	cfgfile = _dyld_get_image_name(0);
	if (stat(cfgfile.c_str(), &st) == 0)
	{
		cfgfile = cann_name(cfgfile);
		append_cfg(cfgfile);
		if (stat(cfgfile.c_str(), &st) == 0)
			return cfgfile;
	}
#endif
	cfgfile = getenv("HOME");
	if (cfgfile.empty()) cfgfile = ".";
	if (cfgfile[cfgfile.length() - 1] != '/') cfgfile += "/";
	cfgfile += ".wrapcc.cfg";
	return cfgfile;
}

static int do_exec(int argc, char** argv)
{
	string cc = safe_env("WRAPCC_CC");
	size_t pos1 = cc.rfind('-') + 1;
	size_t pos2 = cc.rfind('/') + 1;
	size_t pos = std::max<size_t>(pos1, pos2);

	cc.resize(pos);
	cc += argv[0];

	vector<char*> oargs;
	oargs.push_back((char*)cc.c_str());
	for (int i=1; i<argc; ++i)
		oargs.push_back(argv[i]);
	oargs.push_back(nullptr);
	execvp(cc.c_str(), (char**) & oargs[0]);
	perror("execvp");
	return 1;
}

int main(int argc, char ** argv)
{
	string cfgfile;
	if (argc>=2 && strncmp(argv[1], "-config:", 8)==0)
	{
		cfgfile = argv[1] + 8;
		argv[1] = argv[0];
		-- argc;
		++ argv;
	}
	else
	{
		cfgfile = find_configfile();
	}
	loadenv(cfgfile, str2bool(getenv("WRAPCC_CC")));
	if (argc >= 3 && strcmp(argv[1], "--exec") == 0)
	{
		return do_exec(argc-2, argv+2);
	}

	vector<cc_args_group> ags;
	vector<cc_args_group> files;
	int tp = parse_args(ags, files, argc-1, argv+1);

	for (int i=0; i<(int)ags.size(); ++i)
	{
		assert(ags[i].type != cc_args_group::TYPE_INVALID);
	}

	string cc = safe_env("WRAPCC_CC");

	if (! files.empty())
	{
		modify(ags, files, tp);
	}

	for (auto & file : files)
	{
		const char * fn = file.args[0].c_str();
		char buf[1024];
		if (fn[0] == '-')
			strncpy(buf, fn, sizeof(buf));
		else
			sutil::get_full_path(fn, buf, sizeof(buf));
		file.args[0] = buf;
	}

	ags.insert(ags.end(), files.begin(), files.end());

	std::stable_sort(ags.begin(), ags.end(), [](const cc_args_group & a, const cc_args_group & b)->bool
	{
		if (a.type != b.type)
			return a.type < b.type;
		return a.idx < b.idx;
	});

	vector<const char *> oargs;
	oargs.push_back(cc.c_str());
	for (const cc_args_group & ag :  ags)
	{
		if (ag.type == cc_args_group::TYPE_INVALID)
			continue;
		for (const string & s : ag.args)
			oargs.push_back(s.c_str());
	}
	oargs.push_back((const char*)0);

	const char * pdbg = getenv("WRAPCC_DBG");
	string dbg = (pdbg && *pdbg) ? pdbg : safe_env("WRAPCC_DBG");

	unsetenv("WRAPCC_CXXFLAGS");
	unsetenv("WRAPCC_CFLAGS");
	unsetenv("WRAPCC_LDFLAGS");

	unsetenv("WRAPCC_CXXFLAGS2");
	unsetenv("WRAPCC_CFLAGS2");
	unsetenv("WRAPCC_LDFLAGS2");

	unsetenv("WRAPCC_CC");
	unsetenv("WRAPCC_DBG");

	if (!dbg.empty() && dbg!="0")
	{
		FILE * logfp = stderr;
		if (dbg[0] == '/')
		{
			logfp = fopen(dbg.c_str(), "ab");
			if (!logfp) logfp = stderr;
		}

		for (auto i : oargs)
		{
			if (i)
			{
				if (strchr(i, ' '))
					fprintf(logfp, "'%s' ", i);
				else
					fprintf(logfp, "%s ", i);
			}
			else
				break;
		}
		fprintf(stderr, "\n");
	}
	execvp(cc.c_str(), (char**) & oargs[0]);
	perror("execvp\0net.swigger.tool.wrapcc");
	return 1;
}
