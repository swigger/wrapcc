#include "stdafx.h"
#include "cc_args.h"

static int check_single_opt(char ch, bool & hasvalue)
{
	hasvalue = false;
	switch (ch)
	{
		case 'I': //-I /usr/include
		case 'D': //-D macro=v
		case 'U': //-U macro
		case 'x': //-x lang
			hasvalue =1;
			return cc_args_group::TYPE_CCFLAGS;
		case 'l': //-lxml
		case 'u': //symbol
		case 'T': //script.
		case 'L':
		case 'F':
		case 'e': //-e symbol #entry is symbol
			hasvalue = 1;
			return cc_args_group::TYPE_LINKFLAGS;

		case 'O': //-O2
			return cc_args_group::TYPE_CCFLAGS;
		case 's': //strip
			return cc_args_group::TYPE_LINKFLAGS;

		case 'o':
			hasvalue = 1;
			return cc_args_group::TYPE_OTHER;

		case 'G':
		case 'A':
			hasvalue = 1;
			return cc_args_group::TYPE_UNKNOWN;

		default:
			return cc_args_group::TYPE_UNKNOWN;
	}
}

int cc_args_group::in_check(const char ** grp, const char * fnd, int type, int n)
{
	for (int i=0; grp[i]; ++i)
	{
		if (strcmp(fnd, grp[i]) == 0)
		{
			this->type = type;
			return n;
		}
	}
	return -1;
}

int cc_args_group::init(const char * a0)
{
	type = TYPE_INVALID;
	assert(a0[0] == '-');
	if (a0[1] == '-' && a0[2]==0) return -1;

	args.push_back(a0);
	if (a0[1] && a0[2] == 0)
	{
		bool hasv = false;
		type = check_single_opt(a0[1], hasv);
		return hasv;
	}

#define IN_CHECK(a,b,c,d) r=in_check(a,b,c,d); if (r>=0) return r;
	int r = -1;

	static const char * gcc_a0[] = {
		"-undef", "-undefined",
		0
	};
	IN_CHECK(gcc_a0, a0, TYPE_CCFLAGS, 0);

	static const char * clang_is[] = {
		"-include", //file
		"-imacros", //file
		"-idirafter", //dir
		"-iprefix", //prefix
		"-iwithprefix",// dir
		"-iwithprefixbefore",// dir
		"-isysroot",// dir
		"-imultilib",// dir
		"-isystem",// dir
		"-iquote",// dir

		"-MF","-MT","-MQ",
		0
	};
	IN_CHECK(clang_is, a0, TYPE_CCFLAGS, 1);

	static const char * grps[] = {
		"-aspen_version_min",
		"-ios_simulator_version_min",
		"-ios_version_min",
		"-iphoneos_version_min",
		"-macosx_version_min",
		"-mios_version_min",
		"-tvos_simulator_version_min",
		"-tvos_version_min",
		"-watchos_simulator_version_min",
		"-watchos_version_min",
		"-arch", "-target", "-filelist", "-triple",
		"--sysroot",
		0
	};
	IN_CHECK(grps, a0, TYPE_OTHER, 1);

	static const char * lnkf[] = {
		"-Xlinker", "-framework", "-weak_framework", "-install_name", "-multiply_defined",
		"-compatibility_version", "-current_version",
	   	0
	};
	IN_CHECK(lnkf, a0, TYPE_LINKFLAGS, 1);
	if (strcmp(a0, "-sectcreate")==0)
	{
		this->type = TYPE_LINKFLAGS;
		return 3; //macosx ld -sectcreate with 3 more args!!
	}

	static const char * unk[] = {
		"-bundle_loader",// executable
		"-allowable_client", //  client_name
		"-aux-info", // filename
		"--param", //name=value
		0};
	IN_CHECK(unk, a0, TYPE_UNKNOWN, 1);

	static const char * link0[] = {
		"-pie",
		0
	};
	IN_CHECK(link0, a0, TYPE_LINKFLAGS, 0);

	//special case, -Wl,xxxx
	if (strncmp(a0, "-Wl", 3) == 0)
	{
		type = TYPE_LINKFLAGS;
		return 0;
	}

	if (a0[1])
	{
		bool hasv = false;
		int type1 = check_single_opt(a0[1], hasv);
		if (hasv)
		{
			type = type1;
			return 0;
		}
	}

	type = TYPE_UNKNOWN;
	return 0;
}

string file_ext_nover(crefstr file)
{
	string sext;
	const char * fn = file.c_str();
	const char * fne = fn + strlen(fn);

	for (;;)
	{
		const char * fp = 0;
		for (const char * p = fne-1; p>=fn; --p)
		{
			if (*p == '.')
			{
				fp = p;
				break;
			}
			if (*p=='/' || *p=='\\')break;
		}
		if (fp==0) break;
		if (std::all_of(fp+1, fne, isdigit))
		{
			fne = fp;
		}
		else
		{
			sext.assign(fp, fne);
			break;
		}
	}
	return sext;
}

static int check_file_type(crefstr lang, crefstr file)
{
	if (lang.find("c++")!=string::npos ||
		lang.find("cpp")!=string::npos)
		return CTP_CPP;
	else if (lang.length() > 0)
		return CTP_C;

	//check some file is CPP.
	string sext = file_ext_nover(file);
	const char * ext = sext.c_str();
	if (ext)
	{
		if (strcasecmp(ext, ".cc") == 0 ||
			strcasecmp(ext, ".cxx") == 0 ||
			strcasecmp(ext, ".cpp") == 0 ||
			strcasecmp(ext, ".c++") == 0 ||
			strcasecmp(ext, ".mm") == 0)
		{
			return CTP_CPP;
		}
		else if (strcasecmp(ext, ".c") == 0 ||
				 strcasecmp(ext, ".i") == 0 ||
				 strcasecmp(ext, ".m") == 0 ||
				 strcasecmp(ext, ".s") == 0 ||
				 strcasecmp(ext, ".S") == 0)
		{
			return CTP_C;
		}
		else if (strcmp(ext, ".o")==0 ||
				 strcmp(ext, ".so")==0 ||
				 strcmp(ext, ".a")==0 ||
				 strcmp(ext, ".dylib")==0)
		{
			return CTP_IMAGE;
		}
	}
	return CTP_UNKNOWN;
}

bool has_link(const vector<cc_args_group>& ags)
{
	bool is_link = true;
	for (const cc_args_group & cg : ags)
	{
		if (cg.type == cc_args_group::TYPE_INVALID)
			continue;
		//fix: -current_version 1 is not -c
		if (cg.args[0].length() != 2)
			continue;
		char ch = cg.args[0][1];
		if (strchr("cES", ch))
		{
			is_link = false;
			break;
		}
	}
	return is_link;
}

int parse_args(vector<cc_args_group> & ags, vector<cc_args_group> & files, int argc, const char *const* argv)
{
	int btp = CTP_UNKNOWN;
	string lang;
	int cur_filetype = -1;
	int i;

	auto ana_file_type = [&](cc_args_group & a){
		if (a.args.size()==0) return;
		const char * s = a.args[0].c_str();
		if (s[0]=='-' && s[1]=='x')
		{
			const char * tp = s[2] ? s+2 : a.args[1].c_str();
			lang = tp;
			if (strcmp(tp, "none")==0)
			{
				lang = "";
				cur_filetype = -1;
			}
			else
				cur_filetype = cc_args_group::TYPE_SRC_FILE;
		}
	};

	for (i=0; i<argc; ++i)
	{
		if (strcmp(argv[i], "--") == 0)
		{
			break;
		}
		else if (argv[i][0] == '-' && argv[i][1])
		{
			cc_args_group a;
			a.idx = i;
			int nmore = a.init(argv[i]);
			if (nmore < 0) break;
			for (int j=0; j<nmore; ++j)
			{
				a.args.push_back(argv[++i]);
			}
			ags.push_back(a);
			ana_file_type(a);
		}
		else
		{
			cc_args_group af;
			af.idx = i;
			af.type = cur_filetype>=0 ? cc_args_group::TYPE_SRC_FILE : cc_args_group::TYPE_FILE;
			af.args.push_back(argv[i]);

			int ctp = check_file_type(lang, argv[i]);
			if (ctp == CTP_C||ctp==CTP_CPP) btp = ctp;
			else if (ctp == CTP_IMAGE) af.type = cc_args_group::TYPE_LINKFLAGS;
			files.push_back(af);
		}
	}

	for (; i<argc; ++i)
	{
		cc_args_group af;
		af.idx = i;
		af.type = cur_filetype>=0 ? cc_args_group::TYPE_SRC_FILE : cc_args_group::TYPE_FILE;
		af.args.push_back(argv[i]);

		int ctp = check_file_type(lang, argv[i]);
		if (ctp == CTP_C||ctp==CTP_CPP) btp = ctp;
		else if (ctp == CTP_IMAGE) af.type = cc_args_group::TYPE_LINKFLAGS;
		files.push_back(af);
	}
	return btp | (has_link(ags) ? CTP_LINK : 0);
}
