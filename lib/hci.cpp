#include "bluegrass/hci.hpp"

namespace bluegrass {

	inline bool operator<(device_t const& dev, device_t const& other)
	{
		return (long long unsigned int) dev.addr.b < (long long unsigned int) other.addr.b;
	}

	hci::hci() 
	{
		device_ = hci_get_route(NULL);
		socket_ = hci_open_dev(device_);
		
		if (device_ < 0 || socket_ < 0 || hci_devinfo(device_, &info_) < 0) {
			throw std::runtime_error("Failed creating HCI controller");
		}
	}

	hci::~hci() 
	{ 
		std::unique_lock<std::mutex> lock {m_};
		c_close(socket_); 
	}

	void hci::inquiry(size_t max, std::vector<device_t>& devices) 
	{
		std::unique_lock<std::mutex> lock {m_};
		devices.clear();
		
		// allocate temp buffer for inquiry results before transferring to set
		inquiry_info* inquiries = static_cast<inquiry_info*>(
		::operator new[](max * sizeof(inquiry_info)));

		size_t resps = hci_inquiry(device_, 8, max, NULL, 
		(inquiry_info**) &inquiries, IREQ_CACHE_FLUSH);
		
		for (size_t i = 0; i < resps; ++i) {
			devices.push_back({ (inquiries + i)->bdaddr, (inquiries + i)->clock_offset });
		}
		
		::operator delete[](inquiries);
	}

	void hci::inquiry(size_t max, std::vector<bdaddr_t>& devices) 
	{
		std::unique_lock<std::mutex> lock {m_};
		devices.clear();
		
		// allocate temp buffer for inquiry results before transferring to set
		inquiry_info* inquiries = static_cast<inquiry_info*>(
		::operator new[](max * sizeof(inquiry_info)));

		size_t resps = hci_inquiry(device_, 8, max, NULL, 
		(inquiry_info**) &inquiries, IREQ_CACHE_FLUSH);
		
		for (size_t i = 0; i < resps; ++i) {
			devices.push_back({ (inquiries + i)->bdaddr });
		}
		
		::operator delete[](inquiries);
	}

	std::string hci::name(device_t const& dev) const
	{
		std::unique_lock<std::mutex> lock {m_};
		char cstr[64];
		std::string str { "unknown" };
	
		if (hci_read_remote_name(socket_, &(dev.addr), sizeof(cstr), cstr, 0) >= 0) {
			str = cstr;
		}
		
		return str;
	}

	int8_t hci::rssi(device_t& dev) const
	{
		std::unique_lock<std::mutex> lock {m_};
		int8_t rssi;
		int flag { 0 }, conn;
		uint16_t handle;
		
		// get device file descriptor
		conn = hci_get_route(&dev.addr);
		conn = hci_open_dev(conn);
		
		// connect and evaluate RSSI
		flag |= hci_create_connection(conn, &dev.addr, 
		htobs(info_.pkt_type & ACL_PTYPE_MASK), dev.offset, 
		0, &handle, 0);
		flag |= hci_read_rssi(conn, handle, &rssi, 0);
		
		if (flag < 0) {
			rssi = -127;
		}
		// clean up
		hci_disconnect(conn, handle, HCI_OE_USER_ENDED_CONNECTION, 0);
		c_close(conn);
		
		return rssi;
	}

}
