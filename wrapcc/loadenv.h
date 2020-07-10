#pragma once

string cann_name(crefstr pathname);
bool loadenv(crefstr cfgfile, bool useenv);
string safe_env(const char * name);
bool str2bool(const char * p);
