#ifndef __CONTROLLER__
#define __CONTROLLER__

#include <vector>
#include <string>

#include <iostream>

#include "bluegrass/system.hpp"
#include "bluegrass/bluetooth.hpp"

namespace bluegrass {
	
	/*
	 * Class hci_controller provides access to the physical hardware controller
	 * interface on the hardware running the program. hci_controller is a 
	 * singleton which guarantees a program has one path to physical controller.
	 */
	class hci_controller {
	public:
		/*
		 * Description: hci_controller singleton accessor function. Singleton
		 * ensures only one socket connection exists to the physical HCI.
		 */
		static hci_controller& access() 
		{
			static hci_controller hci;
			return hci;
		}
		
		// hci_controller is movable and copyable
		hci_controller(const hci_controller& other) = default;
		hci_controller(hci_controller&& other) = default;
		hci_controller& operator=(const hci_controller& other) = default;
		hci_controller& operator=(hci_controller&& other) = default;
		
		// Performs RAII socket closing
		~hci_controller() { c_close(socket_); }
		
		/*
		 * Function address_inquiry has two parameters:
		 *     max_resps - maximum number of responses to return
		 *     addrs - vector storing the addresses discovered during the inquiry
		 *
		 * Description: address_inquiry makes a blocking call to the physical
		 * HCI which performs an inquiry of nearby broadcasting Bluetooth devices.
		 * A vector of Bluetooth addresses are returned which will be <= max_resps.
		 */
		void address_inquiry(std::size_t max_resps, std::vector<bdaddr_t>& addrs) const
		{
			addrs.clear();
			inquiry_info* inquiries = static_cast<inquiry_info*>(
			::operator new[](max_resps * sizeof(inquiry_info)));
			std::size_t resps = hci_inquiry(device_, 16, max_resps, NULL, 
			(inquiry_info**) &inquiries, IREQ_CACHE_FLUSH);
			
			for(std::size_t i = 0; i < resps; ++i) {
				addrs.push_back((inquiries+i)->bdaddr);
			}
			
			::operator delete[](inquiries);
		}
		
		/*
		 * Function remote_names has two parameters:
		 *     addrs - vector storing the Bluetooth addresses translate
		 *     names - vector storing the human readable remote device names
		 *
		 * Description: remote_names makes blocking calls to the physical
		 * HCI which perform queries of a nearby broadcasting Bluetooth devices
		 * retrieving their human readable device name. If a Bluetooth device 
		 * is unreachable "unknown" will be set for that device. Positions of
		 * readable names will correspond to the position of the Bluetooth address
		 * in the addrs vector.
		 */
		void remote_names(const std::vector<bdaddr_t>& addrs, 
		std::vector<std::string>& names) const
		{
			char str[64];			
			names.clear();
			for(auto addr : addrs) {
				if(hci_read_remote_name(socket_, &addr, sizeof(str), str, 0) < 0) {
					names.push_back(std::string("unknown"));
				} else {
					names.push_back(std::string(str));
				}
			}
		}
	
	private:
		/*
		 * Description: hci_controller provides a simplified interface to the
		 * Bluetooth hardware controller interface. The HCI provides the means
		 * to search for nearby Bluetooth device addresses and device names.
		 */
		hci_controller() 
		{
			device_ = hci_get_route(NULL);
			socket_ = hci_open_dev(device_);
			
			if(device_ < 0 || socket_ < 0) {
				throw std::runtime_error("Failed creating socket to HCI controller");
			}
		}
		
		int device_, socket_;
	};
	
	/*
	 * Class sdp_controller may be used to access the local device SDP server 
	 * or to access the SDP server on a remote device. The default argument of
	 * this class connects to the local device SDP server, but a programmer may
	 * pass the Bluetooth address of remote device to access the device's SDP
	 * server. On destruction the class object closes the SDP server session it
	 * represents.
	 */
	class sdp_controller {
	public:
		/*
		 * Description: sdp_controller() constructs an SDP session with the 
		 * local SDP server of the device running the program. Only 
		 * register_service (will be) invokable by an sdp_controller constructed
		 * in this way.
		 */
		sdp_controller() {
			session_ = sdp_connect((bdaddr_t *) 0, (bdaddr_t *) 0xFFFFFF, SDP_RETRY_IF_BUSY);
		}
		
		/*
		 * Description: sdp_controller(bdaddr_t) constructs an SDP session with 
		 * a remote Bluetooth device's SDP server. Only service_search (will be)
		 * invokable by an sdp_controller object constructed in this way.
		 */
		sdp_controller(bdaddr_t addr) {
			session_ = sdp_connect((bdaddr_t *) 0, &addr, SDP_RETRY_IF_BUSY);
		}
		
		sdp_controller(const sdp_controller& other) = delete;
		sdp_controller(sdp_controller&& other) = default;
		sdp_controller& operator=(const sdp_controller& other) = delete;
		sdp_controller& operator=(sdp_controller&& other) = default;
		
		// will also unregister all services associated with this sdp session
		~sdp_controller() { sdp_close(session_); }
		
		/*
		 * Function service_search has two parameters:
		 *     svc - the service ID to search for (proto and port are not used)
		 *     resps - container to store the found services
		 *
		 * Description: service_search performs a search of services matching
		 * the argument "svc" service on the remote device's SDP server. On 
		 * return the "resps" vector is filled with matching services. The 
		 * majority of code was ported from Albert Huang's: "The Use of 
		 * Bluetooth in Linux and Location aware Computing"
		 */
		void service_search(const service svc, std::vector<service>& resps) const 
		{
			uuid_t id;
			sdp_list_t *resp, *search, *attr, *proto;
			uint32_t range = 0x0000FFFF;
			
			resps.clear();
			sdp_uuid16_create(&id, svc.id);
			search = sdp_list_append(NULL, &id);
			attr = sdp_list_append(NULL, &range);
			
			// perform search on the remote device's SDP server
			if(sdp_service_search_attr_req(
			session_, search, SDP_ATTR_REQ_RANGE, attr, &resp) < 0) {
				throw std::runtime_error("Failed searching for service");
			}
			
			// iteratre list of service records
			for(sdp_list_t* r = resp; r; r = r->next) {
				if(sdp_get_access_protos((sdp_record_t*) r->data, &proto) >= 0) {
					// iterate list of protocol sequences for each service record
					for(sdp_list_t* p = proto; p; p = p->next) {
						// iterate through specific protocols for each sequence
						for(sdp_list_t* pdata = (sdp_list_t*) p->data; 
						pdata; pdata = pdata->next) {
							int pt = 0;
							// iterate through the attributes of a specific protocol
							for(sdp_data_t* pattr = (sdp_data_t*) pdata->data; 
							pattr; pattr = pattr->next) {
								switch(pattr->dtd) {
								case SDP_UUID16:
									pt = sdp_uuid_to_proto(&pattr->val.uuid);
									break;
								case SDP_UINT8:
									resps.push_back(
									{ svc.id, (proto_t) pt, pattr->val.uint16 });
									break;
								}
							}
						}
						
						sdp_list_free((sdp_list_t*) p->data, 0);
					}
					
					sdp_list_free(proto, 0);
				}
				
				sdp_record_free((sdp_record_t*) resp->data);
			}
			
			sdp_list_free(resp, 0);
		}
		
		// todo
		void register_service(const service svc, const std::string name, 
		const std::string description, const std::string provider) {
			
		}
	
	private:
		// todo
		void unregister_service(/* ??? */) {
			
		}
	
		sdp_session_t* session_;
		// vector storing services registered to the sdp session
	};
	
}

#endif
