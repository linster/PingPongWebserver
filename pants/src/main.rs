use std::net::TcpListener;
use std::thread;
use request::Request;

mod request;


fn main() {
    
     const TCP_ADDR: &'static str = "127.0.0.1:8080";
    //create input pool
    
    
    let listener = TcpListener::bind(TCP_ADDR).unwrap();
    
    for stream in listener.incoming(){
        match stream {
            Ok(stream) => {
                thread::spawn(move|| {
                     let req = Request::new(stream); 
                     println!("{}", req.client_addr)
                });
            }
            Err(e) => {
                println!("thread {}", e);
            }
        }
    }
    
    
    //create output threads

    
}