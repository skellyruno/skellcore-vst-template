#pragma once
// Worker-thread-only store for PresetOwnership's local state. The directory is
// chosen by the host (per product, outside the managed feed and user preset files).
#include <SkellCore/PresetOwnership.h>
#include <filesystem>
#include <fstream>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace skellcore::presetownership {
class DiskStore {
    using Path=std::filesystem::path;
    Path root;
    class Lock {
#if defined(_WIN32)
        HANDLE h=INVALID_HANDLE_VALUE;
        OVERLAPPED region{};
#else
        int fd=-1;
#endif
        bool locked=false;
    public:
        explicit Lock(const Path& p) {
#if defined(_WIN32)
            h=CreateFileW(p.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
            if(h!=INVALID_HANDLE_VALUE){BY_HANDLE_FILE_INFORMATION info{};if(GetFileInformationByHandle(h,&info)&&!(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))locked=LockFileEx(h,LOCKFILE_EXCLUSIVE_LOCK,0,1,0,&region)!=0;}
#else
            fd=::open(p.c_str(),O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW,0600);
            if(fd>=0){int rc;do{rc=::flock(fd,LOCK_EX);}while(rc<0&&errno==EINTR);locked=rc==0;}
#endif
        }
        Lock(const Lock&)=delete;Lock& operator=(const Lock&)=delete;
        ~Lock(){
#if defined(_WIN32)
            if(h!=INVALID_HANDLE_VALUE){if(locked)UnlockFileEx(h,0,1,0,&region);CloseHandle(h);}
#else
            if(fd>=0){if(locked)::flock(fd,LOCK_UN);::close(fd);}
#endif
        }
        explicit operator bool()const{return locked;}
    };
    bool prepare()const {
        std::error_code ec;std::filesystem::create_directories(root,ec);
        return !ec&&std::filesystem::is_directory(root,ec)&&!ec;
    }
    std::optional<Bytes> readUnlocked()const {
        const auto path=root/"ownership.bin";std::error_code ec;
        auto st=std::filesystem::symlink_status(path,ec);
        if(st.type()==std::filesystem::file_type::not_found)return Bytes{};
        if(ec||!std::filesystem::is_regular_file(st))return {};
        auto size=std::filesystem::file_size(path,ec);if(ec||size==0||size>stateLimit)return {};
        std::ifstream in(path,std::ios::binary);if(!in)return {};
        Bytes raw(static_cast<std::size_t>(size),'\0');if(!in.read(raw.data(),static_cast<std::streamsize>(size))||in.peek()!=std::char_traits<char>::eof())return {};
        return raw;
    }
    bool replace(std::string_view next)const {
        const auto temp=root/"ownership.new",dest=root/"ownership.bin";std::error_code ec;
        // Recover an interrupted cooperative writer's temporary file. Never follow it.
        auto st=std::filesystem::symlink_status(temp,ec);
        if(st.type()!=std::filesystem::file_type::not_found){if(ec||!std::filesystem::is_regular_file(st)||!std::filesystem::remove(temp,ec)||ec)return false;}
#if defined(_WIN32)
        HANDLE f=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(f==INVALID_HANDLE_VALUE)return false;
        DWORD count=0;bool ok=WriteFile(f,next.data(),static_cast<DWORD>(next.size()),&count,nullptr)&&count==next.size()&&FlushFileBuffers(f);
        CloseHandle(f);if(ok)ok=MoveFileExW(temp.c_str(),dest.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
        int f=::open(temp.c_str(),O_CREAT|O_EXCL|O_WRONLY|O_CLOEXEC|O_NOFOLLOW,0600);if(f<0)return false;
        std::size_t pos=0;bool ok=true;
        while(pos<next.size()){auto n=::write(f,next.data()+pos,next.size()-pos);if(n<0&&errno==EINTR)continue;if(n<=0){ok=false;break;}pos+=static_cast<std::size_t>(n);}
        if(ok)ok=::fsync(f)==0;
        if(::close(f)!=0)ok=false;
        if(ok)ok=::rename(temp.c_str(),dest.c_str())==0;
        if(ok){int dir=::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC);if(dir>=0){::fsync(dir);::close(dir);}}
#endif
        if(!ok)std::filesystem::remove(temp,ec);
        return ok;
    }
public:
    explicit DiskStore(Path directory):root(std::move(directory)){}
    std::optional<Bytes> load()const {
        if(!prepare())return {};
        Lock lock(root/"ownership.lock");if(!lock)return {};
        return readUnlocked();
    }
    bool compareAndSwap(std::string_view old,std::string_view next)const {
        if(old.size()>stateLimit||next.empty()||next.size()>stateLimit||!prepare())return false;
        Lock lock(root/"ownership.lock");if(!lock)return false;
        auto current=readUnlocked();return current&&*current==old&&replace(next);
    }
    // Capture a copy of the path, not this; safe after DiskStore destruction.
    LocalIO io()const {
        return {[store=*this](){return store.load();},[store=*this](auto old,auto next){return store.compareAndSwap(old,next);}};
    }
};
} // namespace skellcore::presetownership
