use std::{
    collections::{HashMap, HashSet},
    net::SocketAddr,
    sync::{Arc, Mutex},
};

use futures_util::{SinkExt, StreamExt};
use serde::{Deserialize, Serialize};
use tokio::net::{TcpListener, TcpStream};

#[serde_with::skip_serializing_none]
#[derive(Clone, Default, Deserialize, Serialize)]
struct PlayerInfo {
    /// ZOne
    zo: Option<String>,
    /// Position X
    px: Option<i64>,
    /// Position Y
    py: Option<i64>,
    /// Position Z
    pz: Option<i64>,
}

impl PlayerInfo {
    fn has_none(&self) -> bool {
        self.zo.is_none() || self.px.is_none() || self.py.is_none() || self.pz.is_none()
    }
}

struct State {
    /// A square vector.
    /// `info[index][index]` contains the "real" info for the player at `index`.
    /// `info[index][other]` contains the "last" version of `other`'s info that was sent to `index`.
    info: Vec<Vec<PlayerInfo>>,
    /// The indexes into `info` that should be skipped, representing players who have disconnected.
    /// These indexes will be recycled on connection if available. Otherwise, `info` will grow.
    skip: HashSet<usize>,
}

impl State {
    fn new() -> Self {
        Self { info: Vec::new(), skip: HashSet::new() }
    }

    /// Adds `info` to vec and returns the `index` that should be used as a param to other functions
    /// in `State`. Initial values for all "last" entries in the vec will be set to default to
    /// indicate that no update has been sent, ensuring that the first update will include all info.
    ///
    /// If a player has previously disconnected, their `index` will be recycled.
    ///
    /// This func will return `Err` if `info` has any fields equal to `None`.
    fn insert(&mut self, info: PlayerInfo) -> Result<usize, String> {
        if info.has_none() {
            Err(String::from("tried to insert info that wasn't fully defined"))
        } else if self.skip.is_empty() {
            // no values to recycle from skip, so the new index goes at the end of info
            let index = self.info.len();

            for other in 0..index {
                self.info[other].push(PlayerInfo::default());
            }

            let mut new_vec = vec![PlayerInfo::default(); index];
            new_vec.push(info);
            self.info.push(new_vec);

            Ok(index)
        } else {
            // recycle any value from skip
            let index = self.skip.iter().next().unwrap().clone();
            self.skip.remove(&index);

            for other in 0..self.info.len() {
                if self.skip_index(index, other) {
                    continue;
                }
                self.info[other][index] = PlayerInfo::default();
                self.info[index][other] = PlayerInfo::default();
            }
            self.info[index][index] = info;

            Ok(index)
        }
    }

    /// Removes `index` from the state. The player that calls this function should not use `index`
    /// to call any other functions in `State`.
    fn remove(&mut self, index: usize) {
        assert!(self.valid_index(index));
        self.skip.insert(index);
    }

    /// Updates the state for any values in `info` that are not `None`.
    fn update(&mut self, index: usize, info: PlayerInfo) {
        assert!(self.valid_index(index));
        let real = &mut self.info[index][index];
        if info.zo.is_some() {
            real.zo = info.zo;
        }
        if info.px.is_some() {
            real.px = info.px;
        }
        if info.py.is_some() {
            real.py = info.py;
        }
        if info.pz.is_some() {
            real.pz = info.pz;
        }
    }

    /// Generates a map of updates for the player at `index`. A particular field is only sent in the
    /// update if the "real" value differs from the "last" value sent. Each `other` valid player is
    /// always included in the map to inform the player that `other` is still connected, even if
    /// there are no updates for that player.
    fn gen_updates(&mut self, index: usize) -> HashMap<usize, PlayerInfo> {
        assert!(self.valid_index(index));
        let mut updates = HashMap::new();
        for other in 0..self.info.len() {
            if self.skip_index(index, other) {
                continue;
            }

            // fairly nasty, but this lets us update only the fields in last that need to be updated
            // while creating the update, which saves us from unnecessarily cloning zone (String)
            let max = std::cmp::max(index, other);
            let (left, right) = self.info.split_at_mut(max);
            let (last, real) = if max == other {
                (&mut left[index][other], &right[0][other])
            } else {
                // max == index
                (&mut right[0][other], &left[other][other])
            };
            let update = PlayerInfo {
                zo: if last.zo != real.zo {
                    last.zo = real.zo.clone();
                    real.zo.clone()
                } else {
                    None
                },
                px: if last.px != real.px {
                    last.px = real.px;
                    real.px
                } else {
                    None
                },
                py: if last.py != real.py {
                    last.py = real.py;
                    real.py
                } else {
                    None
                },
                pz: if last.pz != real.pz {
                    last.pz = real.pz;
                    real.pz
                } else {
                    None
                },
            };
            updates.insert(other, update);
        }
        updates
    }

    fn valid_index(&self, index: usize) -> bool {
        index < self.info.len() && !self.skip.contains(&index)
    }

    fn skip_index(&self, index: usize, other: usize) -> bool {
        index == other || self.skip.contains(&other)
    }
}

async fn handle_connection(state: Arc<Mutex<State>>, raw_stream: TcpStream, addr: SocketAddr) {
    println!("{}: incoming TCP connection", addr);

    let mut ws_stream = tokio_tungstenite::accept_async(raw_stream)
        .await
        .expect("Error during the websocket handshake occured");
    println!("{}: WebSocket connection established", addr);

    // the index into state for this player
    // a value of None indicates the player hasn't sent an initial message
    let mut index: Option<usize> = None;

    while let Some(msg) = ws_stream.next().await {
        // validate message
        let msg = match msg {
            Ok(msg) => msg,
            Err(err) => {
                println!("{}: failed to get next: {}", addr, err);
                break;
            }
        };
        if msg.is_close() {
            println!("{}: received close message", addr);
            break;
        }
        if !msg.is_text() && !msg.is_binary() {
            continue;
        }

        // parse message
        let msg = match msg.to_text() {
            Ok(s) => s,
            Err(err) => {
                println!("{}: failed to cast message to text: {}", addr, err);
                break;
            }
        };
        let info = match serde_json::from_str(msg) {
            Ok(info) => info,
            Err(err) => {
                println!("{}: failed to parse message: {}", addr, err);
                break;
            }
        };

        // update state
        if let Some(index) = index {
            state.lock().unwrap().update(index, info);
        } else {
            index = match state.lock().unwrap().insert(info) {
                Ok(i) => Some(i),
                Err(err) => {
                    println!("{}: failed to insert: {}", addr, err);
                    break;
                }
            }
        }

        // send updates
        // at this point index is guaranteed to be Some, so we can just unwrap
        let updates = state.lock().unwrap().gen_updates(index.unwrap());
        let msg = match serde_json::to_string(&updates) {
            Ok(s) => s,
            Err(err) => {
                println!("{}: failed to serialize updates: {}", addr, err);
                break;
            }
        };
        if let Err(err) = ws_stream.send(msg.into()).await {
            println!("{}: failed to send updates: {}", addr, err);
            break;
        }
    }
    println!("{}: disconnected", addr);

    // cleanup
    if let Some(index) = index {
        state.lock().unwrap().remove(index);
    }
}

#[tokio::main]
async fn main() {
    // TODO read addr from env args
    let addr = String::from("127.0.0.1:8080");
    let listener = TcpListener::bind(&addr).await.expect("Failed to bind");
    println!("Server started, listening on {}", addr);

    let state = Arc::new(Mutex::new(State::new()));
    while let Ok((stream, addr)) = listener.accept().await {
        tokio::spawn(handle_connection(state.clone(), stream, addr));
    }
}
