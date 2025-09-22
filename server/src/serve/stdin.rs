/// Returns true if the exit command was entered.
pub fn handle_command(command: &str) {
    if command == "/exit" {
        println!("terminating server");
        std::process::exit(0);
    }
    println!("unrecognized command");
}
