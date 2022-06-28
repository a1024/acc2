#include		"include/stdio.h"
#include		"include/sys/stat.h"
#include		"include/string"
int				file_is_readable(const char *filename)//0: not readable, 1: regular file, 2: folder
{
#ifdef _DEBUG
	assert(filename);
#endif
	struct stat info;
	int error=stat(filename, &info);
	if(!error)
		return 1+!S_ISREG(info.st_mode);
	return 0;
}
bool			open_bin(const char *filename, std::vector<byte> &data)
{
#ifdef _DEBUG
	assert(filename);
#endif
	struct stat info;
	if(stat(filename, &info)!=-1)
	{
		FILE *file;
		fopen_s(&file, filename, "rb");
		if(file)
		{
			data.resize(info.st_size);
			unsigned readbytes=fread(data.data(), 1, info.st_size, file);
			data.resize(readbytes);
			fclose(file);
			return true;
		}
	}
	return false;
}
bool			open_txt(const char *filename, std::string &data)
{
#ifdef _DEBUG
	assert(filename);
#endif
	struct stat info;
	if(stat(filename, &info)!=-1)
	{
		FILE *file;
		fopen_s(&file, filename, "r");
		if(file)
		{
			data.resize(info.st_size);
			unsigned readbytes=fread(data.data(), 1, info.st_size, file);
			data.resize(readbytes);
			fclose(file);
			return true;
		}
	}
	return false;
}
bool			save_file(const char *filename, bool bin, const void *data, unsigned bytesize)
{
#ifdef _DEBUG
	assert(filename);
#endif
	FILE *file;
	if(bin)
		fopen_s(&file, filename, "wb");
	else
		fopen_s(&file, filename, "w");
	if(file)
	{
		fwrite(data, 1, bytesize, file);
		fclose(file);
		return true;
	}
	return false;
}

bool			open_textfile_lines(const char *filename, std::vector<std::string> &lines)
{
	std::string text;
	open_txt(filename, text);
	if(!text.size())
	{
		printf("Failed to open \'%s\'\n", filename);//
		return false;
	}
	int size=text.size();
	for(int k=0, k0=0, lineno=0;;++k)
	{
		char temp=text[k];
		if(temp=='\n'||!temp)
		{
			int linelen=k-k0;
			if(linelen)
				lines.push_back(std::string(text.c_str()+k0, k-k0));
			k0=k+1;
		}
		if(k>=size||!temp)
			break;
	}
	return true;
}

void			assign_path_or_name(std::string &out, const char *name, int len, bool is_directory)
{
#ifdef _DEBUG
	assert(name);
#endif
	out.assign(name, len);
	//out.resize(len);

	for(int k=0;k<len;++k)//forward slashes only
		if(out[k]=='\\')
			out[k]='/';

	if(len>0)//no quotes
	{
		if(out[0]=='\"')
			out.erase(0);
		if(out[len-1]=='\"')
			out.pop_back();
	}

	int ret=file_is_readable(out.c_str());
	switch(ret)
	{
	case 0:
		printf("Cannot open %s\n", out.c_str());
		break;
	case 1:
		if(is_directory)
			printf("Expected a folder, but %s is a file\n", out.c_str());
		break;
	case 2:
		if(!is_directory)
			printf("Expected a file, but %s is a folder\n", out.c_str());
		break;
	}
	if(is_directory&&out[out.size()-1]!='/')//directories end with slash
		out.push_back('/');
}