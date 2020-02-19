#ifndef MEMBUF_H_
#define MEMBUF_H_

#include <streambuf>

struct membuf : std::streambuf
{
    membuf() {};
    membuf(char* base, std::size_t size);
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
    std::streampos seekpos(std::streampos sp, std::ios_base::openmode which);
};

#endif // MEMBUF_H_
