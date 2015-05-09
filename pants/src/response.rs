
use std::io;
use std::net;

 
enum HTTPresponses {
    OK = 200,
    Created = 201,
    
    MultipleChoices = 300,
    MovedPermanently = 301,
    
    BadRequest = 400,
    NotAuthorized = 401,
    Forbidden = 403,
    FileNotFound = 404,
    RequestTimeout = 408,
    Gone = 410,
    LengthRequired = 411,
    
    InternalServerError = 500,
    NotImplemented = 501
    
}

struct Request {
    client_addr: std::net::SocketAddr,
    req_str: str, //Entire HTTP request.
}


fn ParseRequest(req: &str) {
// http://bofh.srv.ualberta.ca/beck/c379/asg2_p.html
// Our server will have the following format:
// GET /somepath _<space>_ HTTP/1.1
//

let req_str = String::from_str(str);

}

