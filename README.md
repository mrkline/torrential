# Torrential

A BitTorrent simulator by Matt Kline, and Justin Krosschell, and Lee Stratman

## Overview

Torrential was developed as a final project for a Networking class
in the Computer Engineering program at the University of Wisconsin-Madison.
It emulates a set of peers sharing information using the BitTorrent protocol,
so that the protocol's behavior can be studied.

Several simplifying assumptions are made for ease of implementation.
- The simulation is carried out in one-second ticks.
- Upload and download rates are in chunks per tick.
- Due to this granularity, all communicaton between peers
  besides the actual transfer of chunks is assumed to be instant.
These simplifications do not appear to have a negative impact on results,
whose characteristics match those of an actual BitTorrent network.

## Features

- **Multi-threaded**: Torrential utilizes each core to decrease simulation time.
- **Realistic seeding**: Torrential emulates BitTorrent's tit-for-tat mechanism
  for distributing chunks among peers.
  A offer resolution mechanism is used so that peers are not sent duplicate
  chunks each tick and that each peer's bandwidth is used optimally.
  This resembles BitTorrent's similar attempts to avoid duplicate sends.
- **Options**: Torrential accepts a number of parameters, including
  - The number of peers in the simulation
  - The number of chunks in the completed torrent
  - The rates at which peers connect and disconnect from the network
  - The minimum and maximum upload and download rates
    (Rates are taken from these ranges)
  - The number of free riders

## Stats Generator

To assist with analysis of the system and data gathering,
a quick utility was written in D.
Located in the `stat_generator/` directory,
this program takes the number of runs desired as the first argument
followed by any arguments to be passed to torrential.

The stats generator uses the `-m` argument to torrential,
parses the output, and outputs for each run

- The total number of ticks the run took
- The number of ticks an optimal client-server system would take to distribute
  the data to all peers.
- The number of ticks an optimal peer-to-peer system would take to distribute
- A CSV table of each peer with:
  - Peer number
  - Upload rate in chunks/tick
  - Download rate in chunks/tick
  - Number of ticks taken to download all data
  - Number of ticks that the peer was connected to other peers.
  - The number of chunks this peer sent.
