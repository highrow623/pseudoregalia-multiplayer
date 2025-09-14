use std::collections::{HashMap, HashSet};

use serde::{Deserialize, Serialize};

/// The contents of messages passed to and from the server, containing data for a player.
#[serde_with::skip_serializing_none]
#[derive(Clone, Default, Deserialize, Serialize)]
pub struct PlayerInfo {
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

/// Shared state between all threads, used to track what has been received from and what should be
/// sent to players.
pub struct State {
    /// A square vector.
    /// `info[index][index]` contains the "real" info for the player at `index`.
    /// `info[other][index]` contains the "last" version of `other`'s info that was sent to `index`.
    info: Vec<Vec<PlayerInfo>>,
    /// The indexes into `info` that should be skipped, representing players who have disconnected.
    /// These indexes will be recycled on connection if available. Otherwise, `info` will grow.
    skip: HashSet<usize>,
}

impl State {
    pub fn new() -> Self {
        Self { info: Vec::new(), skip: HashSet::new() }
    }

    /// Adds `info` to vec and returns the `index` that should be used as a param to other functions
    /// in `State`. Initial values for all "last" entries in the vec will be set to default to
    /// indicate that no update has been sent, ensuring that the first update will include all info.
    ///
    /// If a player has previously disconnected, their `index` will be recycled.
    ///
    /// This func will return `Err` if `info` has any fields equal to `None`.
    pub fn insert(&mut self, info: PlayerInfo) -> Result<usize, String> {
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
    pub fn remove(&mut self, index: usize) {
        assert!(self.valid_index(index));
        self.skip.insert(index);
    }

    /// Updates the state for any values in `info` that are not `None`.
    pub fn update(&mut self, index: usize, info: PlayerInfo) {
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
    pub fn gen_updates(&mut self, index: usize) -> HashMap<usize, PlayerInfo> {
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
