use crate::state::State;
use std::{
    process,
    sync::{Arc, Mutex},
};

pub fn handle_command(_state: &Arc<Mutex<State>>, command: &str) {
    // We don't use state yet but it could theoretically be used for all sorts of things
    // e.g. a /warp_all command, which would send a message to all clients
    // or a command to get info about currently connected players
    match command {
        "/exit" => {
            println!("terminating server");
            process::exit(0);
        }
        "" => {}
        _ => println!("unrecognized command: {}", command),
    }
}
