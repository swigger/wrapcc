#pragma once

namespace sutil
{
	inline bool s_begin_with(const char * s, const char * sf)
	{
		return strncmp(s, sf, strlen(sf)) == 0;
	}

	inline vector<string> split_str(crefstr ins, crefstr spl, int maxs = -1)
	{
		vector<string> vr;
		if (spl.empty())
		{
			vr.push_back(ins);
			return vr;
		}

		for (size_t pos=0; ; )
		{
			size_t prepos = pos;
			pos = (maxs>1||maxs<0) ? ins.find(spl, prepos) : string::npos;
			if (pos == string::npos)
			{
				vr.push_back(ins.substr(prepos));
				break;
			}
			else
				vr.push_back(ins.substr(prepos, pos-prepos));
			pos += spl.length();
			if (maxs > 0) --maxs;
		}
		return vr;
	}

	inline void remove_empty(vector<string> & args)
	{
		auto it = std::remove(args.begin(), args.end(), "");
		args.erase(it, args.end());
	}
	inline int get_full_path(const char *fn, char * buf, size_t n)
	{
		if (n<=1) return 0;
		if (fn[0] == '/')
		{
			strncpy(buf, fn, n);
			buf[n-1] = 0;
			return 1;
		}
		char* es = getcwd(buf, n);
		if (es == 0 || *es == 0)
			return 0;
		int e = (int) strlen(buf);
		if (buf[e-1] != '/')
		{
			buf[e] = '/';
			buf[++e] = 0;
		}
		strcat(buf, fn);
		return e + (int)strlen(fn) + 1;
	}
}
