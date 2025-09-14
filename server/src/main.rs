use crate::state::State;

use std::{
    net::SocketAddr,
    sync::{Arc, Mutex},
};

use futures_util::{SinkExt, StreamExt};
use tokio::{
    net::{TcpListener, TcpStream},
    sync::mpsc::error::TrySendError,
};

mod state;

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
