use crate::state::{ConnectionUpdate, State};
use futures_util::{SinkExt, StreamExt};
use serde::{Deserialize, Serialize, ser::SerializeMap};
use std::{
    net::SocketAddr,
    sync::{Arc, Mutex},
};
use tokio::net::TcpStream;
use tokio_tungstenite::WebSocketStream;

enum ServerMessage {
    Connected { id: u32, players: Vec<u32> },
    PlayerJoined { id: u32 },
    PlayerLeft { id: u32 },
}

impl Serialize for ServerMessage {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        match self {
            Self::Connected { id, players } => {
                let mut map = serializer.serialize_map(Some(3))?;
                map.serialize_entry("type", "Connected")?;
                map.serialize_entry("id", id)?;
                map.serialize_entry("players", players)?;
                map.end()
            }
            Self::PlayerJoined { id } => {
                let mut map = serializer.serialize_map(Some(2))?;
                map.serialize_entry("type", "PlayerJoined")?;
                map.serialize_entry("id", id)?;
                map.end()
            }
            Self::PlayerLeft { id } => {
                let mut map = serializer.serialize_map(Some(2))?;
                map.serialize_entry("type", "PlayerLeft")?;
                map.serialize_entry("id", id)?;
                map.end()
            }
        }
    }
}

// TODO if this gets more complicated, with different message types and additional fields besides
// type, switch to enum and implement custom Deserialize
#[derive(Deserialize)]
struct ClientMessage {
    #[serde(rename = "type")]
    msg_type: String,
}

/// Cleans up connection by disconnecting id from state on drop
struct DisconnectHandler {
    state: Arc<Mutex<State>>,
    id: u32,
}

impl DisconnectHandler {
    fn new(state: Arc<Mutex<State>>, id: u32) -> Self {
        Self { state, id }
    }
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

    let Some((id, mut rx, players)) = state.lock().unwrap().connect() else {
        // TODO send an error message to the client and allow them to try again
        println!("{}: id collision error", addr);
        return;
    };
    let _disconnect_handler = DisconnectHandler::new(state.clone(), id);

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
            // ignore the number returned because buf is guaranteed to be empty and
            // send_connection_updates drains all of buf
            _ = rx.recv_many(&mut buf, limit) => {
                if let Err(err) = send_connection_updates(&mut ws_stream, &mut buf).await {
                    break format!("failed to send connection updates: {}", err);
                }
            }
            msg = ws_stream.next() => {
                let Some(msg) = msg else {
                    break String::from("reading from stream returned None");
                };
                let msg = match msg {
                    Ok(msg) => msg,
                    Err(err) => {
                        break format!("failed to read next from stream: {}", err);
                    },
                };
                if msg.is_close() {
                    break String::from("received close message");
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
            return Err(String::from("stream reader closed"));
        };
        let msg = msg.map_err(|e| format!("failed to get next: {}", e))?;
        if msg.is_close() {
            return Err(String::from("received close message"));
        }
        if !msg.is_text() && !msg.is_binary() {
            continue;
        }

        // Parse message
        let msg = msg.to_text().map_err(|e| format!("failed to cast message to text: {}", e))?;
        let msg = serde_json::from_str::<ClientMessage>(msg)
            .map_err(|e| format!("failed to deserialize message: {}", e))?;

        if msg.msg_type != "Connect" {
            return Err(String::from("ill-formed command received"));
        }

        return Ok(());
    }
}

async fn send_connection_updates(
    ws_stream: &mut WebSocketStream<TcpStream>,
    buf: &mut Vec<ConnectionUpdate>,
) -> Result<(), String> {
    if buf.len() == 0 {
        return Err(String::from("rx is closed??"));
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
