
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
    req_str: String, //Entire HTTP request.
}

