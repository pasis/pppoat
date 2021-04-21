PPPoAT - PPP over Any Transport
-------------------------------

Important
---------

!!! Consider using https://github.com/pasis/pppoat2 instead of current version !!!

Description
-----------

PPPoAT creates point-to-point tunnel over a transport. The transport can be
any software implementation which sends data from one pppoat instance to another.

PPPoAT features:
  * Multiple network interface types: PPP, TUN/TAP driver
  * Easy platform for implementation of new transports

List of few possible transports:
  * Network protocol from any layer (IP, TCP, XMPP, HTTP, etc)
  * High level logic over a protocol (e.g. traffic encryption, obfuscation,
    sending over multiple paths, etc)
  * Usage of a media without network stack over it (physical port/bus ?)

Available interface modules:
```
  ppp		PPP interface (requires pppd)
  tap		TUN/TAP driver (TAP interface)
  tun		TUN/TAP driver (TUN interface)
  stdio		STDIN/STDOUT tunneling
```

Available transport modules:
```
  udp		Tunnel over UDP
  xmpp		Tunnel over XMPP protocol (Jabber)
```

Example:
The following example creates ppp0 interfaces on each side with IPs 10.0.0.1 and 10.0.0.2:
```
  (server) pppoat -S -m xmpp xmpp.jid=pppoat@domain.com xmpp.passwd=password
  (client) pppoat -m xmpp xmpp.jid=pppoat-slave@domain.com xmpp.to=pppoat@domain.com xmpp.passwd=password
```

/!\ New version is under development
/!\ UDP module contains hardcoded configuration
