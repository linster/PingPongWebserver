use std::net;

struct Listener{
    clientAddr: net::SocketAddr,
    reqType: String,
    reqFile: String,
    httpProto: String,
    userAgent: String,
    rawReq: String,
}

impl Listener {
    
    fn listen (self) {
       
    }
}