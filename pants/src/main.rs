use std::net::{TcpListener, TcpStream};
use std::thread;
use listener::Listener;



mod listener;


fn main() {
    
     const TCP_ADDR: &'static str = "127.0.0.1:8080";
    //create input pool
    
    
    let listener = TcpListener::bind(TCP_ADDR).unwrap();
    
    for stream in listener.incoming(){
        match stream {
            Ok(stream) => {
                thread::spawn(move|| {
                     let listen = Listener::new(stream);   
                });
            }
            Err(e) => {
                println!("{}", e);
            }
        }
    }
    
    
    //create output threads

    
}

fn print(){
    println!("hello world!");
}