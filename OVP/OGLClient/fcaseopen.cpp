/*
Copyright (c) 2009 Keith Bauer

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Changes for OpenOrbiter :
-20220227: isolate casepath and make it return an std::string
*/

#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include "fcaseopen.h"

// r must have strlen(path) + 2 bytes
std::string casepath(char *path)
{
    char r[260];
    size_t l = strlen(path);
    if(l+2>=sizeof(r)) {
        return {};
    }
    char *p = (char *)alloca(l + 1);
    strcpy(p, path);
    size_t rl = 0;
    
    DIR *d;
    if (p[0] == '/')
    {
        d = opendir("/");
        p = p + 1;
    }
    else
    {
        d = opendir(".");
        r[0] = '.';
        r[1] = 0;
        rl = 1;
    }
    
    int last = 0;
    char *c = strsep(&p, "/");
    while (c)
    {
        if (!d)
        {
            return {};
        }
        
        if (last)
        {
            closedir(d);
            return {};
        }
        
        r[rl] = '/';
        rl += 1;
        r[rl] = 0;
        
        struct dirent *e = readdir(d);
        while (e)
        {
            if (strcasecmp(c, e->d_name) == 0)
            {
                //printf("found %s r=%s\n", e->d_name,r);
                strcpy(r + rl, e->d_name);
                rl += strlen(e->d_name);

                closedir(d);
                d = opendir(r);
                
                break;
            }
            
            e = readdir(d);
        }
        
        if (!e)
        {
            strcpy(r + rl, c);
            rl += strlen(c);
            last = 1;
            closedir(d);
            return {};
        }
        
        c = strsep(&p, "/");
    }
    
    if (d) closedir(d);
    return std::string(r);
}
