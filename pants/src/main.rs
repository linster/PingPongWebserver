use std::net::{TcpListener,TcpStream};
use std::thread;
use std::sync::{Arc, Mutex};
use request::Request;
use std::io::Write;

mod request;


fn main() {
    
    const TCP_ADDR: &'static str = "127.0.0.1:8080"; 
    
    let request_vec: Vec<Request> = Vec::new();
    let mutex = Mutex::new(request_vec);
    let atomic = Arc::new(mutex);
     
    //copy pointers to move into threads later
    let req = atomic.clone();
    let res = atomic.clone();
    
    
    //Request threads
     thread::spawn(move|| {
        //create input pool
        let listener = TcpListener::bind(TCP_ADDR).unwrap();

        for stream in listener.incoming(){
            match stream {
                Ok(stream) => {
                    println!("incomming request");
                    let request_vec = req.clone();
                    
                    thread::spawn(move|| {
                        println!("in request thread");
                         let req = Request::new(stream); 
                         request_vec.lock().unwrap().push(req);
                    });
                }
                Err(e) => {
                    println!("thread {}", e);
                }
            }
        }
     });
    
    //Response threads
    thread::spawn(move|| {
        let request_vec = res.clone();
        
        loop {
            if request_vec.lock().unwrap().is_empty() {
                thread::yield_now();
            } else {
                println!("in request thread");
                let request = request_vec.lock().unwrap().pop().unwrap();
                thread::spawn(move|| {
                     let mut stream = TcpStream::connect(request.client_addr).unwrap();
                     let _ = stream.write("hello world!".to_string().as_bytes());
                     println!("lol")
                });
            }
        }
    });
    
    
    loop {}
    
}