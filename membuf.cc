#include "membuf.h"
#include <streambuf>
 
membuf::membuf(char* base, std::size_t size)
{
    this->setp(base, base + size);
    this->setg(base, base, base + size);
}
 
std::streampos membuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which)
{
    char *p;
    switch (which) {
        case std::ios_base::in:
    	    switch (way) {
    	        case std::ios_base::beg:
    	            p = eback() + off;
    	            break;
    	        case std::ios_base::cur:
    	            p = gptr() + off;
    	            break;
    	        case std::ios_base::end:
    	            p = egptr() + off;
    	            break;
    	        default:
    	            p = 0;
    	            break;
    	    }
    	    if(p >= eback() && p <= egptr()) {
    	        setg(eback(), p, egptr());
    	        return std::streampos(p - eback());
    	    } else {
    	        return std::streampos(std::streamoff(-1));
    	    }
    	    break;
        case std::ios_base::out:
    	    switch (way) {
    	        case std::ios_base::beg:
    	            p = pbase() + off;
    	            break;
    	        case std::ios_base::cur:
    	            p = pptr() + off;
    	            break;
    	        case std::ios_base::end:
    	            p = epptr() + off;
    	            break;
    	        default:
    	            p = 0;
    	            break;
    	    }
    	    if(p >= pbase() && p <= epptr()) {
    	        setp(pbase(), epptr());
		pbump(p - pbase());
    	        return std::streampos(p - pbase());
    	    } else {
    	        return std::streampos(std::streamoff(-1));
    	    }
    	    break;
	default:
    	    return std::streampos(std::streamoff(-1));
    }
}

std::streampos membuf::seekpos(std::streampos sp, std::ios_base::openmode which)
{
    char *p;
    switch (which) {
        case std::ios_base::in:
            p = eback() + sp;
            if(p>=eback() && p <=egptr()) {
                setg(eback(), p, egptr());
                return sp;
            } else {
    	        return std::streampos(std::streamoff(-1));
            }
    	    break;
        case std::ios_base::out:
            p = pbase() + sp;
            if(p>=pbase() && p <=epptr()) {
                setp(pbase(), epptr());
		pbump(sp);
                return sp;
            } else {
    	        return std::streampos(std::streamoff(-1));
            }
    	    break;
	default:
	    return std::streampos(std::streamoff(-1));
    }
}
