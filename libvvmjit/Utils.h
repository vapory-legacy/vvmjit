#pragma once

#include <iostream>

// The same as assert, but expression is always evaluated and result returned
#define CHECK(expr) (assert(expr), expr)

#if !defined(NDEBUG) // Debug

namespace dev
{
namespace vvmjit
{

std::ostream& getLogStream(char const* _channel);

}
}

#define DLOG(CHANNEL) ::dev::vvmjit::getLogStream(#CHANNEL)

#else // Release

namespace dev
{
namespace vvmjit
{

struct Voider
{
	void operator=(std::ostream const&) {}
};

}
}


#define DLOG(CHANNEL) true ? (void)0 : ::dev::vvmjit::Voider{} = std::cerr

#endif
