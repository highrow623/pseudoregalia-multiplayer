use crate::state::{PlayerState, STATE_LEN, State};
use std::{
    net::SocketAddr,
    sync::{Arc, Mutex},
};
use tokio::net::UdpSocket;

const MAX_STATES_PER_PACKET: usize = 21;
const MAX_PACKET_LEN: usize = MAX_STATES_PER_PACKET * STATE_LEN;
const _: () = assert!(MAX_PACKET_LEN <= 508);
const _: () = assert!(MAX_PACKET_LEN + STATE_LEN > 508);

// TODO should send_to be put in a tokio::spawn()?
pub async fn handle_packet(
    state: Arc<Mutex<State>>,
    (id, update_num, player_state): (u8, u32, PlayerState),
    udp_socket: Arc<UdpSocket>,
    addr: SocketAddr,
) {
    let Some(updates) = state.lock().unwrap().update(id, update_num, player_state) else {
        return;
    };

    let mut buf = [0u8; MAX_PACKET_LEN];
    let mut states_in_buf = 0;
    for bytes in updates {
        // states_in_buf ranges from 0 to MAX_STATES_PER_PACKET - 1 here so copy target will always
        // be within buf
        let start = states_in_buf * STATE_LEN;
        let end = start + STATE_LEN;
        buf[start..end].copy_from_slice(&bytes[..]);
        states_in_buf += 1;
        if states_in_buf == MAX_STATES_PER_PACKET {
            // because states_in_buf is MAX_STATES_PER_PACKET, we know buf is full
            send_to(udp_socket.clone(), &buf[..], addr).await;
            states_in_buf = 0;
        }
    }
    if states_in_buf != 0 {
        // states_in_buf ranges from 1 to MAX_STATES_PER_PACKET - 1, so end is always within buf
        let end = states_in_buf * STATE_LEN;
        send_to(udp_socket, &buf[..end], addr).await;
    }
}

async fn send_to(udp_socket: Arc<UdpSocket>, buf: &[u8], addr: SocketAddr) {
    if let Err(err) = udp_socket.send_to(buf, addr).await {
        println!("error sending UDP packet: {err}");
        // TODO resend?? idk
    }
}
