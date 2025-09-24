# TODO

## For initial release

* update client to use the new application protocol, test
* finish writing docs
* add license

## Other

* better logging/error handling
* update the server to handle index collisions by sending an error message and allowing the client to resend the `Connect` message
  * also maybe try regenerating index a few times? 32 bits makes for a big id space, so not even retrying once is probably bad
* try reconnecting to the server when an error happens instead of only on scene load
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
* use JSON schema in cpp mod instead of parsing for errors manually

These will probably be done while I update the client:

* make the "message send timeout" calculate correctly?
  * this won't matter if I use UDP for updates, because I'll send an update every frame
* have AddGhostData return the list of ghost ids to remove so that BP_PM_Manager.UpdateGhosts doesn't have to recalculate that
  * this could also just be in the message from the server so the client doesn't have to calculate it at all?
