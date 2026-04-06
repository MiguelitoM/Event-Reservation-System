# Event Reservation System

## Overview

This project implements a distributed event reservation platform using the sockets API, following a client-server architecture.

The system is composed of:
- an **Event Server (ES)**
- a **User client application**

Communication is performed using both **UDP and TCP protocols**, depending on the type of operation.

## Architecture

The system follows a hybrid communication model:

- **UDP**
  - user management (login, logout, unregister)
  - listing operations (myevents, myreservations)

- **TCP**
  - event management (create, close, list)
  - file transfer (event descriptions)
  - reservations

The server runs both UDP and TCP services on the same port and handles multiple clients concurrently.

Concurrency is handled using:
- `select()` for multiplexing
- `fork()` for file transfers

## Features

### User Management
- login / registration
- logout
- password change
- unregister

### Event Management
- create events (with file upload)
- close events
- list events
- show event details (with file transfer)

### Reservations
- reserve seats
- list user reservations

## Protocol

Custom application-layer protocols are implemented over UDP and TCP.

- UDP → lightweight operations (authentication, listing)
- TCP → stateful operations (events, reservations, file transfer)

Messages are space-separated and newline-terminated, with strict formats and status codes (OK, NOK, ERR).

## Implementation

- Language: C / C++
- Sockets API:
  - `socket`, `bind`, `listen`, `accept`
  - `sendto`, `recvfrom`
  - `read`, `write`
- Multiplexing with `select()`
- Process-based concurrency using `fork()`

## Compilation

```bash
make
```

Or compile individually:

```bash
make server
make client
```

Clean build files:

```bash
make clean
```

## Running

Start the server:

```bash
./serverex [-p port] [-v]
```

Run the client:

```bash
./clientex [-n ESIP] [-p port]
```

## Usage

Supported commands:

- `login UID password`
- `create name file date seats`
- `list`
- `show EID`
- `reserve EID value`
- `myevents`
- `myreservations`
- `close EID`
- `logout`
- `exit`

## Notes

- Handles partial reads and writes correctly
- Robust to invalid protocol messages
- Supports multiple concurrent clients
- Designed for distributed execution across different machines
