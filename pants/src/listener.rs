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
        Listener::ParseRequestStr(tcpString.to_string());
        println!("{}", tcpString);

    }
    
    fn listen (self) {
       
    }
    
    
    pub fn ParseRequestStr(req_str: String) {
        //Populate the fields of the listener struct
        //http://www.trevisrothwell.com/2015/01/string-tokenization-in-rust/
        //Now StrExt doesn't exist. Part of std:str 
        
        //let mut terms = req_str.to_string().split(" ");
        
        for term in req_str.split(" ") { //Works only if path isn't missing
            println!("{}",term);
           // match *term {
           //     String::from_str("GET") => {self.reqType = "GET".to_string()},
           //     path @ _ => {self.reqFile = path},
           //     String::from_str("HTTP /1.1") => {self.httpProto = "HTTP /1.1".to_string()}
           // }
        }
    }
    
    
}