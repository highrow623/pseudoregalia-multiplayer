use crate::state::State;
use std::sync::{Arc, Mutex};
use tokio::net::{TcpListener, UdpSocket};

mod serve;
mod state;

#[tokio::main]
async fn main() {
    let addr = std::env::args().nth(1).unwrap_or(String::from("127.0.0.1:8080"));
    let tcp_listener = TcpListener::bind(&addr).await.expect("Failed to bind TCP listener");
    let udp_socket = UdpSocket::bind(&addr).await.expect("Failed to bind UDP socket");
    println!("Server started, listening on {addr}");

    let state = Arc::new(Mutex::new(State::new()));
    tokio::spawn(serve::tcp(state.clone(), tcp_listener));
    tokio::spawn(serve::udp(state.clone(), udp_socket));

    // stdin doesn't spawn a thread so that the main thread blocks on waiting for the /exit command
    // TODO could make main wait on all three and do some graceful shutdown if one thread ends early
    serve::stdin();
}
