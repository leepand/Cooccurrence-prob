#ifndef _ERROR_H_
#define _ERROR_H_

#include <exception>
#include <cassert>
#include <sstream>

inline
void throw_runtime_error( const std::ostream &os )
{
    using namespace std;
    const stringstream &_str = dynamic_cast<const stringstream&>(os);
    throw runtime_error(_str.str());
}

inline
void throw_runtime_error( const std::string &msg )
{
    using namespace std;
    throw runtime_error(msg);
}

#endif

