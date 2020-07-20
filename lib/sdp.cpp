#include "bluegrass/sdp.hpp"
#include "bluegrass/bluetooth.hpp"

namespace bluegrass {

	sdp::sdp() 
	{
		session_ = sdp_connect(&ANY, &LOCAL, SDP_RETRY_IF_BUSY);
	}

	sdp::sdp(bdaddr_t addr) 
	{
		session_ = sdp_connect(&ANY, &addr, SDP_RETRY_IF_BUSY);
	}

	sdp::~sdp() 
	{
		sdp_close(session_); 
	}

	// Majority of code was ported from Albert Huang's: "The Use of Bluetooth in Linux and Location aware Computing"
	void sdp::search(service_t const& svc, std::vector<service_t>& resps) const
	{
		uuid_t id;
		sdp_list_t *resp, *search, *attr, *proto;
		uint32_t range = 0x0000FFFF;
		
		resps.clear();
		sdp_uuid16_create(&id, svc.id);
		search = sdp_list_append(NULL, &id);
		attr = sdp_list_append(NULL, &range);
		
		// perform search on the remote device's SDP server
		if (sdp_service_search_attr_req(
		session_, search, SDP_ATTR_REQ_RANGE, attr, &resp) < 0) {
			throw std::runtime_error("Failed searching for service_t");
		}
		
		// iterate list of service_t records
		for (sdp_list_t* r = resp; r; r = r->next) {
			if (sdp_get_access_protos((sdp_record_t*) r->data, &proto) >= 0) {
				// iterate list of protocol sequences for each service_t record
				for (sdp_list_t* p = proto; p; p = p->next) {
					// iterate through specific protocols for each sequence
					for (sdp_list_t* pdata = (sdp_list_t*) p->data; 
					pdata; pdata = pdata->next) {
						// iterate through the attributes of a specific protocol
						for (sdp_data_t* pattr = (sdp_data_t*) pdata->data; 
						pattr; pattr = pattr->next) {
							switch(pattr->dtd) {
							case SDP_UUID16:
								resps.push_back(
								{svc.id, pattr->val.uint16});
								break;
							case SDP_UINT8:
								resps.push_back(
								{svc.id, pattr->val.uint16});
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

	// Majority of code was ported from Albert Huang's: "The Use of Bluetooth in Linux and Location aware Computing"
	bool sdp::advertise(service_t const& svc, std::string const& name, 
	std::string const& description, std::string const& provider) {
		bool success = true;
		uuid_t svc_uuid, root_uuid, proto_uuid;
		sdp_list_t *root_list, *sub_list, *proto_list, *access_list;
		sdp_data_t *channel;

		// allocate data for service_t record and assign service_t ID
		sdp_record_t* record = sdp_record_alloc();
		sdp_uuid128_create(&svc_uuid, &svc.id);
		sdp_set_service_id(record, svc_uuid);

		// create root data indicating service_t group
		sdp_uuid16_create(&root_uuid, PUBLIC_BROWSE_GROUP);
		root_list = sdp_list_append(0, &root_uuid);
		sdp_set_browse_groups(record, root_list);

		// allocate protocol specific data
		sdp_uuid16_create(&proto_uuid, L2CAP_UUID);
		channel = sdp_data_alloc(SDP_UINT16, &proto_uuid);
		sub_list = sdp_list_append(0, &proto_uuid);
		proto_list = sdp_list_append(0, sub_list);

		// attach protocol data to the service_t record
		access_list = sdp_list_append(0, proto_list);
		sdp_set_access_protos(record, access_list);

		sdp_set_info_attr(record, name.data(), provider.data(), description.data());
		
		if(sdp_record_register(session_, record, 0) == -1) {
			success  = false;
		}

		// clean up un-needed allocated data
		sdp_data_free(channel);
		sdp_list_free(sub_list, 0);
		sdp_list_free(root_list, 0);
		sdp_list_free(access_list, 0);

		return success;
	}

}
