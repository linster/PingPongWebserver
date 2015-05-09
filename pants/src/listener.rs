use std::io::Read;
use std::net;
use std::str;
use std::string;

pub struct Listener{
    clientAddr: net::SocketAddr,
    reqType: String,
    reqFile: String,
    httpProto: String,
    userAgent: String,
    rawReq: String,
}

impl Listener {
    pub fn new (mut stream: net::TcpStream ){
        let mut tcpString: String = String::new();
        stream.read_to_string(&mut tcpString);
        
        println!("{}", tcpString);

    }
    
    fn listen (self) {
       
    }
    
    
    fn ParseRequestStr(self, req_str: &str) {
        //Populate the fields of the listener struct
        //http://www.trevisrothwell.com/2015/01/string-tokenization-in-rust/
        //Now StrExt doesn't exist. Part of std:str 
        
        let mut term = req_str.ToString.Split(" ");
        
        loop { //Works only if path isn't missing
            match term.next().as_ref() {
                "GET" => {self.reqType = "GET"},
                path @ _ => {self.reqFile = path},
                "HTTP /1.1" => {self.httpProto = "HTTP /1.1"}
            }
        }
    }
    
    
}