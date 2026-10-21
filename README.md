<!-- README.md -->

# udp-sockets

A minimal UDP client and server in C, built on POSIX (Berkeley) sockets.

![Demo](docs/demo.png)

## Overview

- **Server:** listens on a UDP port, prints each datagram it receives along with the sender's address, and replies with `ACK: <message>`.
- **Client:** reads lines from the keyboard, sends each one to the server, and prints the reply. If no reply arrives within 3 seconds, it reports a timeout.

UDP is connectionless. There is no `listen()`, `accept()` or `connect()`, and each datagram is independent and may be lost, duplicated or reordered.

## Requirements

- Linux, macOS, or Windows with WSL
- `gcc` and `make`
- Optional, for testing: `netcat` and Wireshark

On Debian/Ubuntu:

```bash
sudo apt update && sudo apt install -y gcc make netcat-openbsd
```

## Build

```bash
git clone https://github.com/ihcnabd/udp-sockets.git
cd udp-sockets
make
```

This produces the `server` and `client` binaries. Run `make clean` to remove them.

## Usage

### Server

```bash
./server [port]
```

| Argument | Default | Description                     |
| -------- | ------- | ------------------------------- |
| `port`   | `8080`  | UDP port to listen on (1-65535) |

The server binds to all network interfaces and runs until you stop it with `Ctrl+C`.

### Client

```bash
./client [server_ip] [port]
```

| Argument    | Default     | Description                                                               |
| ----------- | ----------- | ------------------------------------------------------------------------- |
| `server_ip` | `127.0.0.1` | IPv4 address of the server (dotted notation, hostnames are not supported) |
| `port`      | `8080`      | Server UDP port                                                           |

Type a message and press Enter to send it. Type `quit` (or press `Ctrl+D`) to exit. Empty lines are ignored.

### Example

Terminal 1:

```
$ ./server 8080
UDP server listening on port 8080
Received from 127.0.0.1:53124 -> hello
Received from 127.0.0.1:53124 -> how are you
```

Terminal 2:

```
$ ./client 127.0.0.1 8080
UDP client -> 127.0.0.1:8080 (type 'quit' to exit)
> hello
Server replied: ACK: hello
> how are you
Server replied: ACK: how are you
> quit
```

### Running on two machines

1. On the server machine, find its IP address (`ip addr`) and start `./server 8080`.
2. Allow UDP port 8080 through the firewall.
3. On the other machine, run `./client <server_ip> 8080`.

## Testing without the client

Send a datagram with netcat:

```bash
echo -n "hello" | nc -u -w1 127.0.0.1 8080
```

Expected reply: `ACK: hello`

## Inspecting the traffic

To see the datagrams on the wire, capture the loopback interface in Wireshark with the display filter:

```
udp.port == 8080
```

Each message appears as one UDP packet, and the `ACK: ...` reply as another in the opposite direction. The payload is plain text.

## Protocol

| Property         | Value                                      |
| ---------------- | ------------------------------------------ |
| Transport        | UDP over IPv4                              |
| Request          | One datagram containing the message text   |
| Reply            | One datagram containing `ACK: <message>`   |
| Max message size | 1023 bytes (longer messages are truncated) |
| Client timeout   | 3 seconds                                  |

## Project structure

```
udp-sockets/
├── src/
│   ├── server.c
│   └── client.c
├── docs/
│   └── demo.png
├── Makefile
├── LICENSE
└── README.md
```

## Troubleshooting

| Problem                                    | Cause and fix                                                                                                              |
| ------------------------------------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `bind: Address already in use`             | Another process uses the port. Pick another port or stop the other process.                                                |
| `bind: Permission denied`                  | Ports below 1024 need root. Use a port of 1024 or higher.                                                                  |
| `No reply from server (timeout after 3 s)` | The server is not running, the IP or port is wrong, or a firewall blocks UDP. UDP gives no error when nobody is listening. |
| `Invalid IP address`                       | The client accepts only IPv4 addresses such as `192.168.1.10`, not hostnames.                                              |
| `nc: command not found`                    | Install netcat, or on Windows run the command inside WSL.                                                                  |

## Limitations

This is a simple educational implementation. Plain UDP as used here has:

- **No authentication:** the server answers any sender.
- **No encryption:** messages are sent in clear text.
- **No reliability:** lost datagrams are not retransmitted.
- **No concurrency:** the server handles one datagram at a time, which is enough for light use.
- **IPv4 only.**

## License

MIT, see [LICENSE](LICENSE).

## Author

Houcine Abdaoui, [github.com/ihcnabd](https://github.com/ihcnabd)
