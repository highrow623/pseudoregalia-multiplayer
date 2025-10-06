use serde::{Deserialize, Serialize};

#[derive(Serialize)]
pub struct PlayerInfo {
    pub id: u8,
    pub color: [u8; 3],
}

#[derive(Serialize)]
#[serde(tag = "type")]
pub enum ServerMessage {
    Connected { id: u8, players: Vec<PlayerInfo> },
    PlayerJoined { id: u8, color: [u8; 3] },
    PlayerLeft { id: u8 },
}

#[derive(Deserialize)]
#[serde(tag = "type")]
pub enum ClientMessage {
    Connect { color: [u8; 3] },
}
