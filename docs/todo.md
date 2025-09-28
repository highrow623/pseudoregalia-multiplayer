# TODO

## For initial release

* finish writing docs
* add license

## Other

* I think the error handling on creating the ws/udp socket might cause leaks
* change default port off 8080? 23432 is unassigned and simple
* try reconnecting to the server when an error happens instead of only on scene load
* use JSON schema in cpp mod instead of parsing for errors manually?
  * I've got the schema at client/PseudoregaliaMultiplayerMod/server-message-schema.json if I end up wanting to use it
* better logging/error handling
* update the server to handle index collisions by sending an error message and allowing the client to resend the `Connect` message
  * also maybe try regenerating index a few times? 32 bits makes for a big id space, so not even retrying once is probably bad
* animations?? options to look into:
  * just use animation sequences, send a "best guess" to sync animation state
  * reuse animation bp or player controller, sync whatever data is needed for animations
  * make new animation bp
* also send dream breaker? or at least attach it to ghost sybil when that player is holding it
* name tags or some ui to say who is connected and which level they are in
* add compression and ssl?
* improve setting connect uri, make it runtime configurable?
* share velocity and set the ghosts to update their position based on velocity on frames when a packet is not received
  * could probably bring the send message frequency down a bit (45ms?)
  * maybe have ghosts freeze if an update hasn't happened in a certain amount of time, just so they don't fly off forever if network traffic is really bad
  * this can still be kinda choppy.. better would be to share more state to predict movement better in between messages
  * using a custom player controller/reusing the one from the game could be good here
* add some time syncing? ie delay playing state for a little bit to allow more time to receive packets
* do some graceful shutdown when the `/exit` command is executed, ie close all active connections before ending the program
* have the server generate a key for adding HMACs to UDP packets so the UDP scheme has some measure of security
  * pass the key to the client in the `Connected` message
  * without encrypting, messages would still be readable by outside actors but not forgeable
  * probably wait for ssl to add this
* switch to UDP only?? the overhead on using ws is probably not worth it, but would require a much more complicated protocol
  * improve server message format so it doesn't send unnecessary data, like the transform for players in different zones
