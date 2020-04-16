# CS118 Project 2





Juho Jung
37073388

High level design:
client will be the first one sending a header
server will confirm by sending ACK, and SYN
then the client will start sending each time acknowledging the numbers
once it sent all of them, it will have a F flag and that's how we finish

the biggest issue with this was the client being able to receive. Unfortunately this is where i had to stop due to running out of time.
i had it there the client was now able to receive from the server for acknowledgment but somehow the packet i would get back from the server was
not what i wanted to be. I knew the concept of controlling seq_num, ack_num but couldn't execute in the end.
To solve the issues, i'd say stackoverflow was the biggest help and this youtube video and helped me understand the basics of UDP
https://www.youtube.com/watch?v=9g_nMNJhRVk















Template for for [CSCI3550 Spring 2020 Project 2](https://uno-csci3550.github.io/Project2-Confundo-Description/)

## Makefile

This provides a couple make targets for things.
By default (all target), it makes the `server` and `client` executables.

It provides a `clean` target, and `tarball` target to create the submission file as well.

You will need to modify the `Makefile` to add your userid for the `.tar.gz` turn-in at the top of the file.

## Provided Files

`server.cpp` and `client.cpp` are the entry points for the server and client part of the project.

## Wireshark dissector

For debugging purposes, you can use the wireshark dissector from `tcp.lua`. The dissector requires
at least version 1.12.6 of Wireshark with LUA support enabled.

To enable the dissector for Wireshark session, use `-X` command line option, specifying the full
path to the `tcp.lua` script:

    wireshark -X lua_script:./confundo.lua

To dissect tcpdump-recorded file, you can use `-r <pcapfile>` option. For example:

    wireshark -X lua_script:./confundo.lua -r confundo.pcap
