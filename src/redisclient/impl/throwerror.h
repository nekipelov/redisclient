#pragma once

#include <boost/system/error_code.hpp>
#include <boost/system/system_error.hpp>

namespace redisclient
{

namespace detail
{

inline void throwError(const boost::system::error_code &ec)
{
    boost::system::system_error error(ec);
    throw error;
}

inline void throwIfError(const boost::system::error_code &ec)
{
    if (ec)
    {
        throwError(ec);
    }
}

}

}
