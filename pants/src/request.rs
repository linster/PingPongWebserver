use std::io::Read;
use std::net;

pub struct Request{
    pub client_addr: net::SocketAddr,
    pub req_type: String,
    pub req_path: String,
    pub http_proto: String,
    pub raw_req: String,
}

impl Request {
    pub fn new (mut stream: net::TcpStream ) ->Request {
        let mut tcp_string: String = String::new();
        
        let _ =stream.read_to_string(&mut tcp_string);
        
        let client_addr = stream.peer_addr().unwrap();
        
        let req_string = tcp_string.to_string();
        let raw_req = tcp_string.to_string();
        //println!("{}", raw_req);
        
        //Populate the fields of the listener struct
        //http://www.trevisrothwell.com/2015/01/string-tokenization-in-rust/
        //Now StrExt doesn't exist. Part of std:str 
        let mut lines = req_string.split("\n");
        let line_one = lines.next();
        
        let req_type   = line_one.unwrap().split(" ").next().unwrap().to_string();
        println!("{}", req_type);
        let req_path   = line_one.unwrap().split(" ").nth(1).unwrap().to_string();
        println!("{}", req_path);
        let http_proto = line_one.unwrap().split(" ").nth(2).unwrap().to_string();
        println!("{}", http_proto);
       
        Request{
                client_addr: client_addr,
                req_type:   req_type,
                req_path:   req_path,
                http_proto: http_proto,
                raw_req: raw_req,
               }

    }
}