use std::process;

pub fn handle_command(command: &str) {
    if command == "/exit" {
        println!("terminating server");
        process::exit(0);
    }
    println!("unrecognized command: {}", command);
}
