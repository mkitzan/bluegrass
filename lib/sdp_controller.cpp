#include "bluegrass/sdp_controller.hpp"
#include "bluegrass/system.hpp"
#include "bluegrass/bluetooth.hpp"

namespace bluegrass {

    sdp_controller::sdp_controller() 
    {
        session_ = sdp_connect((bdaddr_t *) 0, (bdaddr_t *) 0xFFFFFF, SDP_RETRY_IF_BUSY);
    }

    sdp_controller::sdp_controller(bdaddr_t addr) 
    {
        session_ = sdp_connect((bdaddr_t *) 0, &addr, SDP_RETRY_IF_BUSY);
    }

    sdp_controller::~sdp_controller() 
    { 
        sdp_close(session_); 
    }

    void sdp_controller::service_search(const service svc, std::vector<service>& resps) const 
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

    // TODO
    void sdp_controller::register_service(const service svc, const std::string name, 
    const std::string description, const std::string provider) {
        
    }

    // TODO
    void sdp_controller::unregister_service(/* ??? */) {
        
    }

}
