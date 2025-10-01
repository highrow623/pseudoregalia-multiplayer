use crate::state::{PlayerState, STATE_LEN, State};
use std::{
    io, process,
    sync::{Arc, Mutex},
};
use tokio::net::{TcpListener, UdpSocket};

mod stdin;
mod tcp;
mod udp;

pub fn stdin(state: Arc<Mutex<State>>) {
    let stdin = io::stdin();
    let mut buf = String::new();
    loop {
        match stdin.read_line(&mut buf) {
            Ok(_) => {
                stdin::handle_command(&state, buf.trim());
                buf.clear();
            }
            Err(err) => {
                println!("terminating server: failed to read stdin: {err}");
                process::exit(1);
            }
        }
    }
}

pub async fn tcp(state: Arc<Mutex<State>>, tcp_listener: TcpListener) -> String {
    loop {
        match tcp_listener.accept().await {
            Ok((stream, _)) => tokio::spawn(tcp::handle_connection(state.clone(), stream)),
            // TODO does this need to return? or can the listener continue to accept connections?
            Err(err) => return format!("failed to accept TCP stream: {err}"),
        };
    }
}

pub async fn udp(state: Arc<Mutex<State>>, udp_socket: UdpSocket) -> String {
    let mut buf = [0u8; STATE_LEN];
    let udp_socket = Arc::new(udp_socket);
    loop {
        match udp_socket.recv_from(&mut buf).await {
            Ok((len, addr)) => {
                if len != STATE_LEN {
                    println!("received UDP packet of the incorrect length: {len}");
                    continue;
                }
                tokio::spawn(udp::handle_packet(
                    state.clone(),
                    PlayerState::from_bytes(buf),
                    udp_socket.clone(),
                    addr,
                ));
            }
            // TODO does this need to return? or can the socket continue to receive packets?
            Err(err) => return format!("failed to read UDP socket: {err}"),
        }
    }
}
