# TODO

* add license
* write the [running the server](../running-the-server.md) guide
* try reconnecting to the server when an error happens instead of only on scene load
* use JSON schema in cpp mod instead of parsing for errors manually?
  * I've got the schema at client/PseudoregaliaMultiplayerMod/server-message-schema.json if I end up wanting to use it
* better logging/error handling
* animations?? options to look into:
  * just use animation sequences, send a "best guess" to sync animation state
  * reuse animation bp and/or player controller, sync whatever data is needed for animations
  * make new animation bp
* also send dream breaker? or at least attach it to ghost sybil when that player is holding it
* name tags or some ui to say who is connected and which level they are in
* add compression and ssl?
* improve setting connect uri, make it runtime configurable?
* do some graceful shutdown when the `/exit` command is executed, ie close all active connections before ending the program
* have the server generate a key for adding HMACs to UDP packets so the UDP scheme has some measure of security
  * pass the key to the client in the `Connected` message
  * without encrypting, messages would still be readable by outside actors but not forgeable
  * probably wait for ssl to add this
* switch to UDP only?? the overhead on using ws is probably not worth it, but would require a much more complicated protocol
  * improve server message format so it doesn't send unnecessary data, like:
    * the transform for players in different zones
    * the transform for players that aren't moving
