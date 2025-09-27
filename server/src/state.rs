use rand::{Rng, SeedableRng, rngs::SmallRng};
use std::collections::HashMap;
use tokio::sync::mpsc::{self, UnboundedReceiver, UnboundedSender};

// arbitrary limit on number of connected players, but this also happens to guarantee that server
// updates fit in one packet
const MAX_PLAYERS: usize = 26;

// the header consists of an update number and the zone, both 4 bytes long
pub const HEADER_LEN: usize = 4;
pub const STATE_LEN: usize = 20;
pub const CLIENT_PACKET_LEN: usize = HEADER_LEN + STATE_LEN;

pub struct PlayerState {
    update_num: u32,
    bytes: [u8; STATE_LEN],
}

impl PlayerState {
    /// Creates a new PlayerState from its byte representation. Also returns the bytes of the header
    /// and the parsed id.
    pub fn from_bytes(bytes: &[u8; CLIENT_PACKET_LEN]) -> ([u8; HEADER_LEN], u8, Self) {
        let header_bytes: [u8; HEADER_LEN] = bytes[..HEADER_LEN].try_into().unwrap();
        let update_num = u32::from_be_bytes(header_bytes[..].try_into().unwrap());

        // id is the byte right after the header
        let id = bytes[HEADER_LEN];
        let bytes = bytes[HEADER_LEN..].try_into().unwrap();

        (header_bytes, id, Self { update_num, bytes })
    }

    fn new() -> Self {
        Self { update_num: 0u32, bytes: [0u8; STATE_LEN] }
    }

    fn update(&mut self, other: Self) -> bool {
        if other.update_num > self.update_num {
            *self = other;
            true
        } else {
            false
        }
    }

    fn has_updated(&self) -> bool {
        self.update_num != 0u32
    }
}

pub enum ConnectionUpdate {
    Connected(u8),
    Disconnected(u8),
}

struct Player {
    state: PlayerState,
    tx: UnboundedSender<ConnectionUpdate>,
}

impl Player {
    fn new(tx: UnboundedSender<ConnectionUpdate>) -> Self {
        Self { state: PlayerState::new(), tx }
    }
}

/// Shared state between all threads, used to track what has been received from and what should be
/// sent to players.
pub struct State {
    players: HashMap<u8, Player>,
    rng: SmallRng,
}

impl State {
    pub fn new() -> Self {
        Self { players: HashMap::new(), rng: SmallRng::from_rng(&mut rand::rng()) }
    }

    pub fn connect(&mut self) -> Option<(u8, UnboundedReceiver<ConnectionUpdate>, Vec<u8>)> {
        if self.players.len() == MAX_PLAYERS {
            return None;
        }

        // player limit means this should be fine, right?
        let id = loop {
            let id = self.rng.random::<u8>();
            if !self.players.contains_key(&id) {
                break id;
            }
        };

        // create list of other players' ids while informing other players of this new connection
        let mut players = Vec::with_capacity(self.players.len());
        for (player_id, player) in &self.players {
            players.push(*player_id);
            // if the corresponding rx has been dropped, it doesn't matter that this message won't
            // get read, so we can ignore the error
            let _ = player.tx.send(ConnectionUpdate::Connected(id));
        }

        let (tx, rx) = mpsc::unbounded_channel();
        self.players.insert(id, Player::new(tx));

        Some((id, rx, players))
    }

    /// Removes the player associated with id from state and informs other players that they
    /// disconnected.
    pub fn disconnect(&mut self, id: u8) {
        if let None = self.players.remove(&id) {
            // TODO this shouldn't happen, right?
            return;
        }

        for player in self.players.values() {
            let _ = player.tx.send(ConnectionUpdate::Disconnected(id));
        }
    }

    /// Updates player state if the id corresponds to a connected player and the update number in
    /// player_state is greater than the current number. Returns true if state was updated.
    pub fn update(&mut self, id: u8, player_state: PlayerState) -> bool {
        // TODO should this be combined with filtered_state and return Option<Vec<[u8; STATE_LEN]>>?
        match self.players.get_mut(&id) {
            Some(player) => player.state.update(player_state),
            None => false,
        }
    }

    /// Returns the current bytes for every player, skipping the player with id matching the `id`
    /// param and other players that haven't been updated yet.
    pub fn filtered_state(&self, id: u8) -> Vec<[u8; STATE_LEN]> {
        let mut filtered_state = Vec::with_capacity(self.players.len());
        for (player_id, player) in &self.players {
            if id == *player_id || !player.state.has_updated() {
                continue;
            }
            filtered_state.push(player.state.bytes);
        }
        filtered_state
    }
}
