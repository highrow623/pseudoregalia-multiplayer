use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};

/// The contents of messages passed to and from the server, containing data for a player.
#[serde_with::skip_serializing_none]
#[derive(Clone, Default, Deserialize, Serialize)]
pub struct PlayerInfo {
    /// ZOne
    zo: Option<String>,
    /// Location X
    lx: Option<i64>,
    /// Location Y
    ly: Option<i64>,
    /// Location Z
    lz: Option<i64>,
    /// Rotation X
    rx: Option<i64>,
    /// Rotation Y
    ry: Option<i64>,
    /// Rotation Z
    rz: Option<i64>,
    /// Scale X
    sx: Option<i64>,
    /// Scale Y
    sy: Option<i64>,
    /// Scale Z
    sz: Option<i64>,
    /// Velocity X
    vx: Option<i64>,
    /// Velocity Y
    vy: Option<i64>,
    /// Velocity Z
    vz: Option<i64>,
    /// Acceleration X
    ax: Option<i64>,
    /// Acceleration Y
    ay: Option<i64>,
    /// Acceleration Z
    az: Option<i64>,
}

/// helper for `PlayerInfo::has_none`
macro_rules! return_true_if_none {
    ($self:ident, $field:ident) => {
        if $self.$field.is_none() {
            return true;
        }
    };
}

/// helper for `PlayerInfo::update`
macro_rules! update_field {
    ($self:ident, $info:ident, $field:ident) => {
        if $info.$field.is_some() {
            $self.$field = $info.$field;
        }
    };
}

/// helper for `PlayerInfo::gen_update`
macro_rules! gen_update_field {
    ($self:ident, $real:ident, $field:ident) => {
        if $self.$field != $real.$field {
            $self.$field = $real.$field;
            $real.$field
        } else {
            None
        }
    };
    ($self:ident, $real:ident, clone $field:ident) => {
        if $self.$field != $real.$field {
            $self.$field = $real.$field.clone();
            $real.$field.clone()
        } else {
            None
        }
    };
}

impl PlayerInfo {
    fn has_none(&self) -> bool {
        return_true_if_none!(self, zo);
        return_true_if_none!(self, lx);
        return_true_if_none!(self, ly);
        return_true_if_none!(self, lz);
        return_true_if_none!(self, rx);
        return_true_if_none!(self, ry);
        return_true_if_none!(self, rz);
        return_true_if_none!(self, sx);
        return_true_if_none!(self, sy);
        return_true_if_none!(self, sz);
        return_true_if_none!(self, vx);
        return_true_if_none!(self, vy);
        return_true_if_none!(self, vz);
        return_true_if_none!(self, ax);
        return_true_if_none!(self, ay);
        return_true_if_none!(self, az);
        false
    }

    /// Updates each field in self if that field in info is Some
    fn update(&mut self, info: Self) {
        update_field!(self, info, zo);
        update_field!(self, info, lx);
        update_field!(self, info, ly);
        update_field!(self, info, lz);
        update_field!(self, info, rx);
        update_field!(self, info, ry);
        update_field!(self, info, rz);
        update_field!(self, info, sx);
        update_field!(self, info, sy);
        update_field!(self, info, sz);
        update_field!(self, info, vx);
        update_field!(self, info, vy);
        update_field!(self, info, vz);
        update_field!(self, info, ax);
        update_field!(self, info, ay);
        update_field!(self, info, az);
    }

    /// Updates self to match real and returns a copy that reports which fields were updated.
    fn gen_update(&mut self, real: &Self) -> Self {
        Self {
            zo: gen_update_field!(self, real, clone zo),
            lx: gen_update_field!(self, real, lx),
            ly: gen_update_field!(self, real, ly),
            lz: gen_update_field!(self, real, lz),
            rx: gen_update_field!(self, real, rx),
            ry: gen_update_field!(self, real, ry),
            rz: gen_update_field!(self, real, rz),
            sx: gen_update_field!(self, real, sx),
            sy: gen_update_field!(self, real, sy),
            sz: gen_update_field!(self, real, sz),
            vx: gen_update_field!(self, real, vx),
            vy: gen_update_field!(self, real, vy),
            vz: gen_update_field!(self, real, vz),
            ax: gen_update_field!(self, real, ax),
            ay: gen_update_field!(self, real, ay),
            az: gen_update_field!(self, real, az),
        }
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
        real.update(info);
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
            let update = last.gen_update(real);
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
