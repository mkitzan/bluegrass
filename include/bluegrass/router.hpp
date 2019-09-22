#ifndef __BLUEGRASS_ROUTER__
#define __BLUEGRASS_ROUTER__

#include <vector>

#include "bluegrass/bluetooth.hpp"
#include "bluegrass/hci_controller.hpp"
#include "bluegrass/sdp_controller.hpp"
#include "bluegrass/server.hpp"

namespace bluegrass {

	template <proto_t P>
	class router {
	public:


	private:
		std::vector<bluegrass::server<P>> network_;
	};
	
} // namespace bluegrass 

#endif
