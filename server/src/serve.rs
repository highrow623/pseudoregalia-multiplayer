use crate::state::{CLIENT_PACKET_LEN, PlayerState, State};
use std::sync::{Arc, Mutex};
use tokio::net::{TcpListener, UdpSocket};

mod stdin;
mod tcp;
mod udp;

pub fn stdin() {
    let stdin = std::io::stdin();
    let mut buf = String::new();
    loop {
        match stdin.read_line(&mut buf) {
            Ok(_) => {
                stdin::handle_command(buf.trim());
                buf.clear();
            }
            Err(err) => {
                println!("error reading stdin: {}", err);
                return;
            }
        }
    }
}

pub async fn tcp(state: Arc<Mutex<State>>, tcp_listener: TcpListener) {
    while let Ok((stream, addr)) = tcp_listener.accept().await {
        tokio::spawn(tcp::handle_connection(state.clone(), stream, addr));
    }
    // TODO at least print something here? or panic?
}

pub async fn udp(state: Arc<Mutex<State>>, udp_socket: UdpSocket) {
    let mut buf = [0u8; CLIENT_PACKET_LEN];
    let udp_socket = Arc::new(udp_socket);
    loop {
        match udp_socket.recv_from(&mut buf).await {
            Ok((len, addr)) => {
                if len != CLIENT_PACKET_LEN {
                    println!("Received UDP packet of the incorrect length: {}", len);
                    continue;
                }
                let (id, player_state) = PlayerState::from_bytes(&buf);
                tokio::spawn(udp::handle_packet(
                    state.clone(),
                    id,
                    player_state,
                    udp_socket.clone(),
                    addr,
                ));
            }
            Err(err) => {
                println!("Error reading UDP socket: {}", err);
                // return; // ?
            }
        }
    }
}
