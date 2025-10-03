# Application Protocol

Communication between client and server consists of both WebSocket messages and UDP packets. The client first establishes a WebSocket connection, which the server uses to send important but less frequent updates. The client and server then trade UDP packets with the frame-by-frame data to sync state.

I've decided to go with this for now because I like using UDP for the state updates, but I didn't want to write my own "connection based, in order, guaranteed delivery" protocol on top of UDP. I can get that from WebSockets relatively easily (from a dev perspective anyway). This is admittedly kinda lazy, and it would probably be good to switch to UDP only at some point, but this works for now.

# WebSocket Scheme

All messages are in JSON format, with a `type` field to indicate its purpose, as well as any other fields specific to that message type. For example, a `Connected` packet (described below) might look like this:

```json
{
  "type": "Connected",
  "id": 11,
  "players": [
    57,
    84,
    38
  ]
}
```

## Client to Server Messages

### `Connect`

The `Connect` message is sent after the WebSocket connection is established. This message has no additional fields, but may be updated in the future to validate the connection with a password or set a name for the player.

## Server to Client Messages

### `Connected`

The `Connected` message is sent in response to the `Connect` message to signal a successful connection.

| Field | Type | Description |
| --- | --- | --- |
| `id` | unsigned 8-bit integer | The id assigned to the player |
| `players` | array of unsigned 8-bit integers | The ids of all other currently connected players |

### `PlayerJoined`

The `PlayerJoined` message is sent when a new player connects to the server.

| Field | Type | Description |
| --- | --- | --- |
| `id` | unsigned 8-bit integer | The id of the player that just joined |

### `PlayerLeft`

The `PlayerLeft` message is sent when a connected player disconnects from the server.

| Field | Type | Description |
| --- | --- | --- |
| `id` | unsigned 8-bit integer | The id of the player that just left |

# UDP Scheme

## Client to Server Packets

After establishing a WebSocket connection and receiving a `Connected` packet, clients send a UDP packet every frame to inform the server of their current state. The update is 24 bytes long and has the following format:

* Player id (unsigned 8-bit integer, 1 byte): the id of the player that was received in the `Connected` packet. The server rejects the packet if the id does not match a connected player.
* Milliseconds (unsigned 32-bit integer, 4 bytes): this represents the number of milliseconds between when the client started sending updates to now. The server keeps the most recent N updates. (Currently, N = 20.)
* Zone (unsigned 32-bit integer, 4 bytes): a hash of the zone the player is in. The hash is calculated client-side and used by the client to determine whether another player is in the same zone.
* Transform (15 bytes):
  * Location: the location component of the player's transform, represented by three 32-bit floating point numbers (12 bytes).
  * Rotation: the rotation component of the player's transform, represented by three unsigned 8-bit integers (3 bytes).

Notes:

* Each number in the update is in big endian format.
* After player id and milliseconds, the server doesn't do anything with the data except store it and pass it along to other players.
* Each value in the rotation component of the transform stays between -180.0 and 180.0. The update translates that to an unsigned 8-bit integer, so -180 would map to 0 and just under 180 would map to 255.
* I did a bit of testing and found that the scale component of the transform seems to always be (1.0, 1.0, 1.0), so it is not included in the update.

## Server to Client Packets

Once an update is accepted by the server, the server sends one or more UDP packets with the state of other connected players. An update is `24 * num_updates` bytes long. Each update is in the same format as a client to server packet, with at most one update per player per packet, and a server packet just looks like several player updates in a row. When responding to a client packet, the server will send the most recent update it hasn't already tried to send for each other player.

Notes:

* `num_updates` will always be between 1 and 21, inclusive. So an update will have minimum length 24 and maximum length 504, and the length of an update mod 24 will always be 0.
* Currently the server caps the number of players at 22 so that all player updates will fit in a single packet, but both the client and server should correctly handle updates being sent over multiple packets.
* This format sends unnecessary data, as it will still send the transform for a player in a different zone. This could be improved, but would require a more complicated message format. I'll come back to this later.

The client keeps track of the most recent N updates for each player (currently, N = 20). It calculates the average difference between its own millisecond counter and that of each other player to determine which update to play each frame.
