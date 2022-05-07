#include "ModuleAPI.h"

#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

//#define WINDOWS_DL

namespace oapi {

DynamicModule::DynamicModule() noexcept {
    m_Opaque = nullptr;
}

DynamicModule::DynamicModule(const char *path) noexcept {
#ifdef WINDOWS_DL
    m_Opaque = LoadLibrary(path);
#else
    m_Opaque = dlopen(path, RTLD_NOW|RTLD_GLOBAL|RTLD_DEEPBIND);
#endif
    if(!m_Opaque) {
        fprintf(stderr, "%s\n", dlerror());
        fprintf(stderr, "DynamicModule::DynamicModule: cannot load %s\n", path);
        abort();
        exit(-1);
    }
    m_Path = path;
}

DynamicModule::~DynamicModule() noexcept {
    if(m_Opaque)
#ifdef WINDOWS_DL
        FreeLibrary((HMODULE)m_Opaque);
#else
        dlclose(m_Opaque);
#endif
}

bool DynamicModule::Load(const char *path) noexcept {
    if(m_Opaque) {
        fprintf(stderr, "DynamicModule::Load: cannot load %s over already loaded module\n", path);
        abort();
        exit(-1);
    }
#ifdef WINDOWS_DL
    m_Opaque = LoadLibrary(path);
#else
    m_Opaque = dlopen(path, RTLD_NOW|RTLD_GLOBAL|RTLD_DEEPBIND);
#endif
    if(!m_Opaque) {
        fprintf(stderr, "%s\n", dlerror());
        fprintf(stderr, "DynamicModule::Load: cannot load %s\n", path);
        abort();
//        return false;
        exit(-1);
    }
    m_Path = path;
    dlerror();

    void (*InitModule)(DynamicModule *) = (void(*)(DynamicModule *))(*this)["InitModule"];
    if(InitModule)
        InitModule(this);

    return true;
}

void DynamicModule::Unload() noexcept {
    if(m_Opaque)
#ifdef WINDOWS_DL
        FreeLibrary((HMODULE)m_Opaque);
#else
        dlclose(m_Opaque);
#endif
    dlerror();
    m_Opaque = nullptr;
}

void *DynamicModule::operator[](const char *func) noexcept {
    if(m_Opaque == nullptr) {
        fprintf(stderr, "DynamicModule::operator[] on empty module\n");
        abort();
        exit(-1);
    }
#ifdef WINDOWS_DL
    void *ret =(void*)GetProcAddress((HMODULE)m_Opaque, func);
#else
    void *ret =(void*)dlsym(m_Opaque, func);
#endif
    if(ret == nullptr) {
        //fprintf(stderr, "%s\n", dlerror());
        //fprintf(stderr, "DynamicModule: cannot find symbol '%s'\n", func);
        return nullptr;
    }
    return ret;
};


std::map<std::string, std::pair<DynamicModule, int>> ModuleLoader::m_Modules;

DynamicModule *ModuleLoader::Load(const char *path) {
    std::string s = path;
    if(m_Modules.find(s) != m_Modules.end()) {
        m_Modules[s].second++;
        return &m_Modules[s].first;
    } else {
        DynamicModule m;
        //m.Load(path);
       // if(m.Loaded()) {
    		//char *(*mdate)() = (char*(*)())m["ModuleDate"];
	    	//int (*fversion)() = (int(*)())m["GetModuleVersion"];
	    	//void (*InitModule)(void *) = (void(*)(void *))m["InitModule"];

//            m_Modules.emplace(s, std::make_pair(std::move(m), 1));
            m_Modules.emplace(s, std::make_pair<DynamicModule, int>({}, 1));
            DynamicModule &ret = m_Modules[s].first;
            ret.Load(path);
            if(!ret.Loaded()) {
                printf("Cannot load module %s\n", path);
                abort();
                exit(-1);
            }

            return &ret;
       /* } else {
            printf("Cannot load module %s\n", path);
            abort();
            exit(-1);
            return nullptr;
        }*/
    }
}

void ModuleLoader::Unload(DynamicModule *m) {
    if(m == nullptr) {
        printf("Unloading nullptr!\n");
        abort();
        exit(-1);
    }
    std::string s = m->m_Path;
    if(m_Modules.find(s) != m_Modules.end()) {
        m_Modules[s].second--;
        if(m_Modules[s].second == 0) {
            m_Modules.erase(s);
        }
    } else {
        printf("Cannot unload module %s, not loaded!\n", s.c_str());
        abort();
        exit(-1);
    }
}

};
