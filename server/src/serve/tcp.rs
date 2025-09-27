use crate::state::{ConnectionUpdate, State};
use futures_util::{SinkExt, StreamExt};
use serde::{Deserialize, Serialize};
use std::{
    net::SocketAddr,
    sync::{Arc, Mutex},
};
use tokio::net::TcpStream;
use tokio_tungstenite::WebSocketStream;

#[derive(Serialize)]
#[serde(tag = "type")]
enum ServerMessage {
    Connected { id: u8, players: Vec<u8> },
    PlayerJoined { id: u8 },
    PlayerLeft { id: u8 },
}

#[derive(Deserialize)]
#[serde(tag = "type")]
enum ClientMessage {
    Connect,
}

/// Cleans up connection by disconnecting id from state on drop
struct DisconnectHandler {
    state: Arc<Mutex<State>>,
    id: u8,
}

impl Drop for DisconnectHandler {
    fn drop(&mut self) {
        self.state.lock().unwrap().disconnect(self.id);
    }
}

pub async fn handle_connection(state: Arc<Mutex<State>>, raw_stream: TcpStream, addr: SocketAddr) {
    println!("{}: incoming TCP connection", addr);

    let mut ws_stream = match tokio_tungstenite::accept_async(raw_stream).await {
        Ok(ws_stream) => ws_stream,
        Err(err) => {
            println!("{}: error during WebSocket handshake occured: {}", addr, err);
            return;
        }
    };
    println!("{}: established WebSocket connection", addr);

    if let Err(err) = receive_connect_message(&mut ws_stream).await {
        println!("{}: failed to receive connect message: {}", addr, err);
        return;
    }
    println!("{}: received Connect message", addr);

    let Some((id, mut rx, players)) = state.lock().unwrap().connect() else {
        // TODO send an error message to the client and allow them to try again
        println!("{}: id collision error", addr);
        return;
    };
    let _disconnect_handler = DisconnectHandler { state: state.clone(), id };

    let msg = ServerMessage::Connected { id, players };
    let msg = serde_json::to_string(&msg).unwrap();
    if let Err(err) = ws_stream.send(msg.into()).await {
        println!("{}: error sending connected message: {}", addr, err);
        return;
    }

    // arbitrary, but probably more than enough to handle all updates available at a time
    let limit = 16;
    let mut buf = Vec::with_capacity(limit);
    let reason = loop {
        tokio::select! {
            // ignore the number returned because buf is guaranteed to be empty as
            // send_connection_updates drains all of buf
            _ = rx.recv_many(&mut buf, limit) => {
                if let Err(err) = send_connection_updates(&mut ws_stream, &mut buf).await {
                    break format!("failed to send connection updates: {}", err);
                }
            }
            msg = ws_stream.next() => {
                let Some(msg) = msg else {
                    break "stream reader closed".to_owned();
                };
                let msg = match msg {
                    Ok(msg) => msg,
                    Err(err) => {
                        break format!("failed to read next from stream: {}", err);
                    },
                };
                if msg.is_close() {
                    break "received close message".to_owned();
                }
            }
        }
    };
    println!("{}: disconnected: {}", addr, reason);
}

async fn receive_connect_message(ws_stream: &mut WebSocketStream<TcpStream>) -> Result<(), String> {
    loop {
        // Validate message
        let Some(msg) = ws_stream.next().await else {
            return Err("stream reader closed".to_owned());
        };
        let msg = msg.map_err(|e| format!("failed to get next: {}", e))?;
        if msg.is_close() {
            return Err("received close message".to_owned());
        }
        if !msg.is_text() && !msg.is_binary() {
            continue;
        }

        // Parse message
        let msg = msg.to_text().map_err(|e| format!("failed to cast message to text: {}", e))?;
        // Currently, the only valid client message is Connect, which has no fields, so we only care
        // if deserialization is successful.
        // TODO if more client message types get added or the Connect message gets fields added to
        // it, msg will have to actually be used.
        let _msg = serde_json::from_str::<ClientMessage>(msg)
            .map_err(|e| format!("failed to deserialize message: {}", e))?;

        return Ok(());
    }
}

async fn send_connection_updates(
    ws_stream: &mut WebSocketStream<TcpStream>,
    buf: &mut Vec<ConnectionUpdate>,
) -> Result<(), String> {
    if buf.len() == 0 {
        // I don't think this should ever happen because rx is only dropped when we disconnect
        return Err("rx is closed??".to_owned());
    }
    for connection_update in buf.drain(..) {
        feed_connection_update(ws_stream, connection_update).await?;
    }
    ws_stream.flush().await.map_err(|e| format!("failed to send connection updates: {}", e))
}

async fn feed_connection_update(
    ws_stream: &mut WebSocketStream<TcpStream>,
    connection_update: ConnectionUpdate,
) -> Result<(), String> {
    let msg = match connection_update {
        ConnectionUpdate::Connected(id) => ServerMessage::PlayerJoined { id },
        ConnectionUpdate::Disconnected(id) => ServerMessage::PlayerLeft { id },
    };
    let msg = serde_json::to_string(&msg).unwrap();
    ws_stream.feed(msg.into()).await.map_err(|e| format!("failed to feed connection update: {}", e))
}
