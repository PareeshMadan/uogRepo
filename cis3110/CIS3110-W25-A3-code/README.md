#  Pareesh Madan
#  ID: 1271446
#  Assignment 3
## CIS\*3110/Operating Systems
## W25

----------------------------------------------------------------

#  Brief Discussion describing your program design

This program simulates a token ring network using semaphores as well as threads to send and recive multiple packets among different processes which represent nodes. My program design involved implementing the suggested algorithmn structure suggested in the assignment details.

When a packet is received:
• if the packet is the “available” token
	– if the node wishes to transmit data
		* send the data packet(s)
		* note that the “available” token needs to be passed along after the last bytes of the packet
	– else
		* send the token as is
• else
	– if packet is from this node
		* discard packet
		* if token needs to be passed along
			· send token
		* else
			· retransmit the data packet, noting if it was to this node

Nodes transition through different states to process the packets.

My program contains deadlocke avoidance using by establishing sempahores to enforce a strict ordering

The program is scalable as it works with different node and packet amounts

# Running the program

Compile: make
Execute: ./tokensim <# packets>
