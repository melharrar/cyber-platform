# cyber-platform

The goal of this project is to build a MULTI-PROCESSES platform around dpdk.

DPDK defines two types of processes: primary and secondary.
  - Primary process must be unique in the Linux OS and have all the rights to build/use/remove the DPDK ressources (like pools, queues, rings, eth ports ect ...).
  - Secondary process(es) can only use the ressources created by primary process.

The main vision is to make this platform "Lego based", since the modularity is the key of a good design.

At this time the roadmap is to make an inline platform with the folowings binaries:
  - "AppsManager" as a primary process: Its goal is to build and initialize the ressources.
  - "Dispacher" as secondary process: Its goal is to filter the packets, and dispatch them to the output rings.
  - "TCPClient" and "TCPServer" as secondary process: To be used as statefull injector/generator.
  - "TCPProxy" as secondary process: To be used in inline configuration and be "in the middle".

Some new updates are coming.
