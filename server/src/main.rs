use crate::state::State;
use std::{
    env, process,
    sync::{Arc, Mutex},
    thread,
};
use tokio::net::{TcpListener, UdpSocket};

mod serve;
mod state;

#[tokio::main]
async fn main() {
    let addr = env::args().nth(1).unwrap_or("127.0.0.1:23432".to_owned());
    let tcp_listener = TcpListener::bind(&addr).await.expect("Failed to bind TCP listener");
    let udp_socket = UdpSocket::bind(&addr).await.expect("Failed to bind UDP socket");
    println!("Server started, listening on {addr}");

    let state = Arc::new(Mutex::new(State::new()));
    let tcp_task = tokio::spawn(serve::tcp(state.clone(), tcp_listener));
    let udp_task = tokio::spawn(serve::udp(state.clone(), udp_socket));

    // stdin gets its own thread because it requires blocking calls in order to read inputs
    thread::spawn(move || serve::stdin(state));

    let reason = tokio::select! {
        join_result = tcp_task => {
            match join_result {
                Ok(reason) => format!("TCP task ended: {}", reason),
                Err(err) => format!("TCP task crashed: {}", err),
            }
        }
        join_result = udp_task => {
            match join_result {
                Ok(reason) => format!("UDP task ended: {}", reason),
                Err(err) => format!("UDP task crashed: {}", err),
            }
        }
    };
    println!("terminating server: {}", reason);
    process::exit(1);
}
