#ifndef GSLAM_SharedLibrary_INCLUDED
#define GSLAM_SharedLibrary_INCLUDED

#include <iostream>
#include <set>
#include <mutex>
#include <memory>

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows

#include <windows.h>
#include <io.h>
// A list of annoying macros to #undef.
// Feel free to extend as required.
#undef GetBinaryType
#undef GetShortPathName
#undef GetLongPathName
#undef GetEnvironmentStrings
#undef SetEnvironmentStrings
#undef FreeEnvironmentStrings
#undef FormatMessage
#undef EncryptFile
#undef DecryptFile
#undef CreateMutex
#undef OpenMutex
#undef CreateEvent
#undef OpenEvent
#undef CreateSemaphore
#undef OpenSemaphore
#undef LoadLibrary
#undef GetModuleFileName
#undef CreateProcess
#undef GetCommandLine
#undef GetEnvironmentVariable
#undef SetEnvironmentVariable
#undef ExpandEnvironmentStrings
#undef OutputDebugString
#undef FindResource
#undef UpdateResource
#undef FindAtom
#undef AddAtom
#undef GetSystemDirector
#undef GetTempPath
#undef GetTempFileName
#undef SetCurrentDirectory
#undef GetCurrentDirectory
#undef CreateDirectory
#undef RemoveDirectory
#undef CreateFile
#undef DeleteFile
#undef SearchPath
#undef CopyFile
#undef MoveFile
#undef ReplaceFile
#undef GetComputerName
#undef SetComputerName
#undef GetUserName
#undef LogonUser
#undef GetVersion
#undef GetObject

#else // Linux

#include <unistd.h>
#include <dlfcn.h>

// Note: cygwin is missing RTLD_LOCAL, set it to 0
#if defined(__CYGWIN__) && !defined(RTLD_LOCAL)
#define RTLD_LOCAL 0
#endif

#endif

namespace sv {
class SharedLibrary
    /// The SharedLibrary class dynamically
    /// loads shared libraries at run-time.
{
    enum Flags
    {
        SHLIB_GLOBAL_IMPL = 1,
        SHLIB_LOCAL_IMPL  = 2
    };
public:
    typedef std::mutex MutexRW;
    typedef std::unique_lock<std::mutex> WriteMutex;
    SharedLibrary():_handle(NULL){}
        /// Creates a SharedLibrary object.

    SharedLibrary(const std::string& path):_handle(NULL)
    {
        load(path);
    }
        /// Creates a SharedLibrary object and loads a library
        /// from the given path.

    virtual ~SharedLibrary(){
        if(isLoaded())
        {
#ifndef NDEBUG
//            std::cerr<<"SharedLibrary "<<_path<<"  released."<<std::endl;
#endif
        }
    }
        /// Destroys the SharedLibrary. The actual library
        /// remains loaded.

    bool load(const std::string& path,int flags=0)
    {

        WriteMutex lock(_mutex);

        if (_handle)
            return false;

#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows

        flags |= LOAD_WITH_ALTERED_SEARCH_PATH;
        _handle = LoadLibraryExA(path.c_str(), 0, flags);
        if (!_handle) return false;
#else
        int realFlags = RTLD_LAZY;
        if (flags & SHLIB_LOCAL_IMPL)
            realFlags |= RTLD_LOCAL;
        else
            realFlags |= RTLD_GLOBAL;
        _handle = dlopen(path.c_str(), realFlags);
        if (!_handle)
        {
            const char* err = dlerror();
            std::cerr<<"Can't open file "<<path<<" since "<<err<<std::endl;
            return false;
        }

#endif
        _path = path;
        return true;
    }
        /// Loads a shared library from the given path.
        /// Throws a LibraryAlreadyLoadedException if
        /// a library has already been loaded.
        /// Throws a LibraryLoadException if the library
        /// cannot be loaded.

    void unload()
    {
        WriteMutex lock(_mutex);

        if (_handle)
        {
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows
            FreeLibrary((HMODULE) _handle);
#else
            dlclose(_handle);
#endif
            _handle = 0;
            _path.clear();
        }
    }
        /// Unloads a shared library.

    bool isLoaded() const
    {
        return _handle!=0;
    }
        /// Returns true iff a library has been loaded.

    bool hasSymbol(const std::string& name)
    {
        return getSymbol(name)!=0;
    }
        /// Returns true iff the loaded library contains
        /// a symbol with the given name.

    void* getSymbol(const std::string& name)
    {
        WriteMutex lock(_mutex);

        void* result = 0;
        if (_handle)
        {
#if (defined(_WIN32) || defined(_WIN64)) && !defined(__CYGWIN__)// Windows
            return (void*) GetProcAddress((HMODULE) _handle, name.c_str());
#else
            result = dlsym(_handle, name.c_str());
#endif
        }
        return result;
    }
        /// Returns the address of the symbol with
        /// the given name. For functions, this
        /// is the entry point of the function.
        /// Throws a NotFoundException if the symbol
        /// does not exist.

    const std::string& getPath() const
    {
        return _path;
    }
        /// Returns the path of the library, as
        /// specified in a call to load() or the
        /// constructor.

    static std::string suffix()
    {
#if defined(__APPLE__)
        return ".dylib";
#elif defined(hpux) || defined(_hpux)
        return ".sl";
#elif defined(WIN32) || defined(WIN64)
        return ".dll";
#else
        return ".so";
#endif
    }
        /// Returns the platform-specific filename suffix
        /// for shared libraries (including the period).
        /// In debug mode, the suffix also includes a
        /// "d" to specify the debug version of a library.

private:
    SharedLibrary(const SharedLibrary&);
    SharedLibrary& operator = (const SharedLibrary&);
    MutexRW     _mutex;
    std::string _path;
    void*       _handle;
};
typedef std::shared_ptr<SharedLibrary> SharedLibraryPtr;


} // namespace pi


#endif // PIL_SharedLibrary_INCLUDED
