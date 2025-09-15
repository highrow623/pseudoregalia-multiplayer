use crate::state::State;
use std::sync::{Arc, Mutex};
use tokio::net::TcpListener;

mod serve;
mod state;

#[tokio::main]
async fn main() {
    // TODO read addr from env args
    let addr = String::from("127.0.0.1:8080");
    let listener = TcpListener::bind(&addr).await.expect("Failed to bind");
    println!("Server started, listening on {addr}");

    let state = Arc::new(Mutex::new(State::new()));
    while let Ok((stream, addr)) = listener.accept().await {
        tokio::spawn(serve::handle_connection(state.clone(), stream, addr));
    }
}
