#include <iostream>

#include "bluegrass/bluetooth.hpp"

namespace bluegrass {

	std::ostream& operator<<(std::ostream& out, bdaddr_t const& ba)  
	{
		std::ios_base::fmtflags ff(out.flags());
		out << std::hex << std::uppercase 
		<< (uint32_t) ba.b[5] << ':' << (uint32_t) ba.b[4] << ':' << (uint32_t) ba.b[3] 
		<< ':' 
		<< (uint32_t) ba.b[2] << ':' << (uint32_t) ba.b[1] << ':' << (uint32_t) ba.b[0];
		out.flags(ff);

		return out;
	}

}

