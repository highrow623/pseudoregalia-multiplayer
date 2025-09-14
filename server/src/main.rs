use std::{
    collections::{HashMap, HashSet},
    net::SocketAddr,
    sync::{Arc, Mutex},
};

use futures_util::{SinkExt, StreamExt};
use serde::{Deserialize, Serialize};
use tokio::{
    net::{TcpListener, TcpStream},
    sync::mpsc::error::TrySendError,
};

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
    /// `info[other][index]` contains the "last" version of `other`'s info that was sent to `index`.
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
        } else if self.info.is_empty() {
            // Initial insert
            self.info.push(vec![info]);
            Ok(0)
        } else if self.skip.is_empty() {
            // No values to recycle from skip, so the new index goes at the end of info
            let index = self.info.len();

            for other in 0..index {
                self.info[other].push(PlayerInfo::default());
            }

            let mut new_vec = vec![PlayerInfo::default(); index];
            new_vec.push(info);
            self.info.push(new_vec);

            Ok(index)
        } else {
            // Recycle any value from skip
            let index = self.skip.iter().next().unwrap().clone();
            self.skip.remove(&index);

            for other in 0..self.info.len() {
                if self.skip_other(index, other) {
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
            if self.skip_other(index, other) {
                continue;
            }

            // Fairly nasty, but this lets us update only the fields in last that need to be updated
            // while creating the update, which saves us from unnecessarily cloning zone, a String.
            let max = std::cmp::max(index, other);
            let (left, right) = self.info[other].split_at_mut(max);
            let (last, real) = if max == other {
                (&mut left[index], &right[0])
            } else {
                // max == index
                (&mut right[0], &left[other])
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

    fn skip_other(&self, index: usize, other: usize) -> bool {
        index == other || self.skip.contains(&other)
    }
}

async fn handle_connection(state: Arc<Mutex<State>>, raw_stream: TcpStream, addr: SocketAddr) {
    println!("{addr}: incoming TCP connection");

    let ws_stream = tokio_tungstenite::accept_async(raw_stream)
        .await
        .expect(format!("{addr}: error during the websocket handshake occured").as_str());
    println!("{addr}: established WebSocket connection");
    let (mut ws_write, mut ws_read) = ws_stream.split();

    // Used by main thread to tell write thread when an update is ready to send, sends copy of index
    let (tx, mut rx) = tokio::sync::mpsc::channel(1);

    // Spawn write thread
    let state2 = state.clone();
    let join = tokio::spawn(async move {
        while let Some(index) = rx.recv().await {
            let updates = state2.lock().unwrap().gen_updates(index);
            let msg = match serde_json::to_string(&updates) {
                Ok(msg) => msg,
                Err(err) => {
                    println!("{addr}: failed to serialize updates: {err}");
                    break;
                }
            };
            if let Err(err) = ws_write.send(msg.into()).await {
                println!("{addr}: failed to send updates: {err}");
                break;
            }
        }
    });

    // The index into state for this player. A value of None indicates the player hasn't sent an
    // initial message.
    let mut index = None;

    while let Some(msg) = ws_read.next().await {
        // Validate message
        let msg = match msg {
            Ok(msg) => msg,
            Err(err) => {
                println!("{addr}: failed to get next: {err}");
                break;
            }
        };
        if msg.is_close() {
            println!("{addr}: received close message");
            break;
        }
        if !msg.is_text() && !msg.is_binary() {
            continue;
        }

        // Parse message
        let msg = match msg.to_text() {
            Ok(s) => s,
            Err(err) => {
                println!("{addr}: failed to cast message to text: {err}");
                break;
            }
        };
        let info = match serde_json::from_str(msg) {
            Ok(info) => info,
            Err(err) => {
                println!("{addr}: failed to deserialize message: {err}");
                break;
            }
        };

        // Update state
        if let Some(index) = index {
            state.lock().unwrap().update(index, info);
        } else {
            index = match state.lock().unwrap().insert(info) {
                Ok(index) => Some(index),
                Err(err) => {
                    println!("{addr}: failed to insert: {err}");
                    break;
                }
            };
        }

        // Inform write thread to send update.
        // After updating state, index is guaranteed to be Some, so we can just unwrap.
        if let Err(err) = tx.try_send(index.unwrap())
            && let TrySendError::Closed(_) = err
        {
            // We don't care about a Full error because that just means we've already queued an
            // update. But a Closed error means the write thread has dropped rx and is therefore no
            // longer running.
            println!("{addr}: write thread has stopped running");
            break;
        }
    }

    // Cleanup
    drop(tx);
    let _ = join.await;
    if let Some(index) = index {
        state.lock().unwrap().remove(index);
    }
    println!("{addr}: disconnected");
}

#[tokio::main]
async fn main() {
    // TODO read addr from env args
    let addr = String::from("127.0.0.1:8080");
    let listener = TcpListener::bind(&addr).await.expect("Failed to bind");
    println!("Server started, listening on {addr}");

    let state = Arc::new(Mutex::new(State::new()));
    while let Ok((stream, addr)) = listener.accept().await {
        tokio::spawn(handle_connection(state.clone(), stream, addr));
    }
}
