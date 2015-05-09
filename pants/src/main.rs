extern crate threadpool;

use threadpool::ThreadPool;




fn main() {
    const MAX_THREADS: usize = 10;
    
    //create input pool
    let pool = ThreadPool::new(MAX_THREADS);
    
    for i in 0..5 {
        println!("{}",i);
        pool.execute(move || print(i));
    }
    
    //create output threads

    
    loop {
        
    }
}

fn print(i:u32){
    println!("hello world! {}", i);
}