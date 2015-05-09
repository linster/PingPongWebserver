use std::net;
use std::str;

struct Listner{
    clientAddr: net::SocketAddr,
    reqType: str,
    reqFile: str,
    httpProto: str,
    userAgent: str,
    rawReq: str,
}

impl Listner {
    fn listen () {
        
    }
    
    
    fn ParseRequestStr(self, req_str: str) {
        //Populate the fields of the listener struct
        //http://www.trevisrothwell.com/2015/01/string-tokenization-in-rust/
        //Now StrExt doesn't exist. Part of std:str 
        
        let mut term = str::Split(req_str, " ");
        
        loop {
            match term.next() {
                "GET" => {self.reqType = "GET"},
                path @ _ => {self.reqFile = path},
                "HTTP /1.1" => {self.httpProto = "HTTP /1.1"}
            }
        }
    }
}