#include "stdafx.h"
#include "loadenv.h"
#include <assert.h>

enum e_VOP {OP_UNKNOWN, OP_ADD, OP_ASSIGN};


struct shreader
{
	enum VERB_TYPE{
		VT_END = 0,
		VT_SPACE,
		VT_COMMENT,
		VT_ALNUM,
		VT_OP,
		VT_STRING,
		VT_OTHER,
	};

	const char * ptr, * ptr_begin;

	shreader(const char * p)
	{
		ptr = p;
		ptr_begin = p;
	}

	int read_verb(string & verb)
	{
		if (*ptr == 0) return 0;

		if (isspace(*ptr))
		{
			const char * p0 = ptr++;
			while (isspace(*ptr)) ++ptr;
			verb.assign(p0, ptr-p0);
			return VT_SPACE;
		}

		if (isalnum(*ptr) || *ptr == '_')
		{
			const char * p0 = ptr++;
			while (isalnum(*ptr) || *ptr=='_' || *ptr=='$')
			{
				++ ptr;
			}
			verb.assign(p0, ptr-p0);
			return VT_ALNUM;
		}

		if (*ptr == '+' && ptr[1] == '=')
		{
			ptr += 2;
			verb = "+=";
			return VT_OP;
		}
		if (*ptr == '=')
		{
			++ ptr;
			verb = "=";
			return VT_OP;
		}
		if (*ptr == '#')
		{
			const char * p0  = ptr;
			while (*ptr && *ptr != '\n') ++ ptr;
			verb.assign(p0, ptr-p0);
			return VT_COMMENT;
		}
		if (*ptr == '\'' || *ptr == '"')
		{
			char fnd = *ptr++;
			verb.clear();
			while (*ptr && *ptr != fnd)
			{
				verb += *ptr ++;
			}

			if (*ptr == fnd)
			{
				++ptr;

				for (;;)
				{
					const char * ptrsave = ptr;
					string v1;
					int t = read_verb(v1);
					if (t == VT_STRING || t == VT_ALNUM)
						verb += v1;
					else
					{
						ptr = ptrsave;
						return VT_STRING;
					}
				}
			}
			//error: unmatched string at end.
			return VT_STRING;
		}

		verb = *ptr ++;
		return VT_OTHER;
	}

	int peek_verb(string & verb)
	{
		const char * savep = ptr;
		int r = read_verb(verb);
		ptr = savep;
		return r;
	}

};


static string convert(const char * p,  map<string,string> & vars)
{
	string sr;
	while (*p)
	{
		if (*p == '$')
		{
			const char * p0 = ++p;
			while (*p && (isalnum(*p) || *p=='_') )
				++p;
			string name(p0, p-p0);
			auto it = vars.find(name);
			if (it !=  vars.end())
				sr += it->second;
		}
		else if (*p=='\r' || *p=='\n')
		{
			sr += ' ';
			++p;
		}
		else
		{
			sr += *p ++;
		}
	}
	return sr;
}

static bool parse_nvs(crefstr ind, map<string,string> & omp)
{
	bool r = true;
	shreader rd(ind.c_str());

	string verb;
	for (;;)
	{
		int t = rd.read_verb(verb);
		if (t == rd.VT_ALNUM)
		{
			const char * p = rd.ptr;
			string v1, last_v1;
			string value;
			bool add = false;

			t = rd.read_verb(v1);
			if (t != rd.VT_OP)
			{
				goto skipline;
			}

			add = v1[0] == '+';
			v1.clear();
			for (;;)
			{
				p = rd.ptr;
				t = rd.read_verb(v1);
				if (t == shreader::VT_END || t==shreader::VT_SPACE || t==shreader::VT_COMMENT)
				{
					rd.ptr = p;
					value += convert(last_v1.c_str(), omp);
					break;
				}
				if (p[0] == '\'')
				{
					value += convert(last_v1.c_str(), omp);
					value += v1;
					last_v1.clear();
				}
				else if (p[0] == '\"')
				{
					value += convert(last_v1.c_str(), omp);
					value += convert(v1.c_str(), omp);
					last_v1.clear();
				}
				else
				{
					last_v1 += v1;
				}
			}

			if (add)
				omp[verb] += value;
			else
				omp[verb] =  value;

			//skip this line.
			for (;;)
			{
				p = rd.ptr;
				t = rd.read_verb(v1);
			skipline:
				if (t == shreader::VT_SPACE && v1.find('\n') != string::npos)
					break;
			}
		}
		else if (t == shreader::VT_END)
			break;
	}
	return r;
}

#ifdef _WIN32
string cann_name(crefstr pathname)
{
	//this is still buggy. link not readed.
	char buf[4096] = { 0 };
	GetFullPathNameA(pathname.c_str(), sizeof(buf), buf, 0);
	return buf;
}
#else
static string cann_name_i(crefstr pathname, bool & haslnk)
{
	char buf[4096];

	string oname;
	const char * ptr = pathname.c_str();
	while (*ptr)
	{
		if (*ptr == '/')
		{
			oname += '/';
			ptr ++;
		}
		else if (*ptr == '.' && (ptr[1] == '/' || ptr[1]==0))
		{
			ptr += ptr[1] ? 2 : 1;
		}
		else if (*ptr =='.' && ptr[1]=='.' && (ptr[2]=='/'||ptr[2]==0))
		{
			assert(oname.size()==0 || oname[oname.length()-1] == '/');
			if (oname.empty() || oname=="/")
				;
			else
			{
				for (size_t i=oname.size()-2; ;  )
				{
					if (oname[i] == '/')
						oname.resize(i+1);
					if (i==0) break;
					--i;
				}
			}
		}
		else
		{
			while (*ptr  && *ptr != '/')
				oname += *ptr++;

			memset(buf, 0, sizeof(buf));
			if (readlink(oname.c_str(), buf, sizeof(buf)) > 0)
			{
				haslnk = true;
				if (buf[0] == '/')
					oname = buf;
				else
				{
					while (!oname.empty() && oname[oname.length()-1] != '/')
						oname.resize(oname.length()-1);
					if (oname.empty()) oname = "/";
					oname += buf;
				}
			}
		}
	}
	return oname;
}
string cann_name(crefstr pathname)
{
	if (pathname.empty() || pathname[0] != '/')
	{
		char buf[1024] = { 0 };
		getcwd(buf, sizeof(buf));
		int len = (int)strlen(buf);
		if (len == 0 || buf[len - 1] != '/')
			buf[len++] = '/';
		string name1(buf);
		name1 += pathname;
		return cann_name(name1);
	}
	string path1 = pathname;
	for (int i=0; i<10; ++i)
	{
		bool haslnk = false;
		path1 = cann_name_i(path1, haslnk);
		if (!haslnk) return path1;
	}
	//too many links.
	return string();
}
#endif



static bool g_useenv = false;
static map<string,string> g_nvs;

string xReadFile(const char * path)
{
	string sa;
	FILE * fp = fopen(path, "rb");
	if (!fp) return sa;
	fseek(fp, 0, SEEK_END);
	ssize_t l = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (l>0)
	{
		sa.resize(l);
		l = fread(&sa[0], 1, l, fp);
		sa.resize(l<0 ? 0 : l);
	}
	fclose(fp);
	return sa;
}

bool loadenv(crefstr cfgfile, bool useenv)
{
	g_useenv = useenv;
	if (useenv) return true;

	string cfgdata = xReadFile(cfgfile.c_str());
	if (cfgdata.empty()) return false;

	//set proper CONFIG_DIR
	string name = cann_name(cfgfile);
	size_t pn = name.rfind('/');
	if (pn != string::npos)
		name.resize(pn);
	else
		name = "";

	map<string,string> omp;
	// predefined variables put here.
	omp["CONFIG_DIR"] = name;
	omp["HOME"] = getenv("HOME");
	parse_nvs(cfgdata, omp);
	g_nvs.swap(omp);
	return true;
}

string safe_env(const char * name)
{
	if (!name) return "";

	if (g_useenv)
	{
		const char * p = getenv(name);
		return p ? p : "";
	}
	else
	{
		auto it = g_nvs.find(name);
		if (it != g_nvs.end())
			return it->second;
		else
			return "";
	}
}

bool str2bool(const char * ptr)
{
	if (!ptr || !*ptr) return false;
	if (strcasecmp(ptr, "no") == 0 ||
		strcasecmp(ptr, "n") == 0 ||
		strcasecmp(ptr, "false") == 0 )
		return false;
	char * eptr = 0;
	long long v = strtoll(ptr, &eptr, 0);
	if ((eptr==0 || *eptr==0) && v==0)
		return false;
	return true;
}
