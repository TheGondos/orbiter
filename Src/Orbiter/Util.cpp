// Copyright (c) Martin Schweiger
// Licensed under the MIT License

#include "Util.h"

uint64_t NameToId (const char *name)
{
	uint64_t id = 0;
	const char *c;
	char *cid = (char*)&id;
	int i = 0;
	for (c = name; *c; c++, i++) cid[i%8] += toupper (*c);
	return id;
}

uint64_t Str2Crc (const char *str)
{
	uint64_t crc = 0;
	for (const char *c = str; *c; c++)
		crc += (uint64_t)*c;
	return crc;
}

double rand1()
{
	static double irmax = 1.0/(double)RAND_MAX;
	return (double)rand()*irmax;
}

char *uscram (const char *str)
{
	static char cbuf[4096];
	char *c;
	int k;
	uint8_t key = str[0];
	for (k = 1, c = cbuf; k < 4096 && (str[k] || str[k+1]); k++)
		*c++ = (k&1 ? str[k]+key : str[k]-key);
	*c = '\0';
	return cbuf;
}
#include <sys/stat.h>
bool MakePath (const char *fname)
{
	int ret = mkdir(fname,0755);
	if(ret != 0) {
		printf("mkdir %s failed\n", fname);
		exit(-1);
	}
	return ret==0;
}
