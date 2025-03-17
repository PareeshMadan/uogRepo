/*
* Pareesh Madan
* ID: 1271446
* 
* The program simulates a Token Ring LAN by forking off a process
* for each LAN node, that communicate via shared memory, instead
* of network cables. To keep the implementation simple, it jiggles
* out bytes instead of bits.
*
* It keeps a count of packets sent and received for each node.
*/
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "tokenRing.h"
#include <pthread.h>
#include <string.h>

/*
* This function is the body of a child process emulating a node.
*/
void *
token_node(void * arguments)
{
	//get the arguments
	int num = ((struct ArgumentsToken *)arguments)->num;
	struct TokenRingData *control = ((struct ArgumentsToken *)arguments)->control;

	//variables for the state
	int rcv_state = TOKEN_FLAG, not_done = 1, sending = 0, len;
    unsigned char byte;
	//variables for the packet
    int next_node = (num + 1) % N_NODES;
    int curr = 0, getting = 0;
    struct data_pkt rcv_pkt = {0};

    //get the ball rolling by initializing the token
    if (num == 0) {
		//send the token
        control->snd_state = TOKEN_FLAG;
        send_byte(control, num, '1');
		//signal the next node
        SIGNAL_SEM(control, FILLED(next_node));
    }
	//loop until the node is done
    while (not_done) {
		//wait until the node is filled
        WAIT_SEM(control, FILLED(num));
		//wait until the node is empty
        WAIT_SEM(control, EMPTY(next_node));
		//wait for the critical section
        WAIT_SEM(control, CRIT);
		//receive the byte
        byte = rcv_byte(control, num);
		//check if the node is done
        if (control->shared_ptr->node[num].terminate) {
			//set the flag to 0
            not_done = 0;
        }

        //check the flags if the byte is 1 and the node is not sending
        if (byte == '1' && sending == 0) {
			//set the state to token flag
            rcv_state = TOKEN_FLAG;
		//if the byte is 0
        } else if (byte == '0') {
			//set the state to TO
            rcv_state = TO;
            if (sending==0) {
                send_byte(control, num, byte);
            }
        }

        //if not handling '0' and not sending, then handle the byte
        if (!(byte == '0' && sending==0)) {
			//switch for the state
            switch (rcv_state) {
				//if the state is token flag
                case TOKEN_FLAG:
					//if sending, send the packet
                    if (sending==1) {
                        send_pkt(control, num);
					//otherwise, retransmit the byte
                    } else {
						//if the byte is the node number
                        if (control->shared_ptr->node[num].to_send.length > 0) {
							//send the packet
                            control->snd_state = TOKEN_FLAG;
                            sending = 1;
                            send_pkt(control, num);
                        } else {
							//send the byte
                            send_byte(control, num, '1');
                        }
                    }
					//set the state to TO
                    rcv_state = TO;
                    break;

                case TO:
					//if sending, send the packet
                    if (sending==1) {
                        send_pkt(control, num);
					//otherwise, retransmit the byte
                    } else {
						//if the byte is the node number
                        if (byte == num) {
							//get the packet
							getting = 1;
                            rcv_pkt.to = byte;
                        }
						//send the byte
                        send_byte(control, num, byte);
                    }
					//set the state to FROM
                    rcv_state = FROM;
                    break;
				//if the state is FROM
                case FROM:
					//if sending, send the packet
                    if (sending==1) {
                        send_pkt(control, num);
                    } else {
						//if receiving, get the packet
                        if (getting==1) {
                            rcv_pkt.from = byte;
                        }
						//send the byte
                        send_byte(control, num, byte);
                    }
					//set the state to LEN
                    rcv_state = LEN;
                    break;
				//if the state is LEN
                case LEN:
					//if sending, send the packet
                    if (sending==1) {
                        send_pkt(control, num);
                    } else {
						//if receiving, get the packet
                        if (getting==1) {
                            rcv_pkt.length = byte;
                        }
						//send the byte
                        len = byte;
						curr = 0;
                        send_byte(control, num, byte);
                    }
					//set the state to DATA
                    rcv_state = DATA;
                    break;

                case DATA:
					//if sending and done, send the packet
                    if (sending && control->snd_state == DONE) {
						//clear the packet and flags
                        sending = 0;
                        rcv_state = TOKEN_FLAG;
                        control->snd_state = TOKEN_FLAG;
                        control->shared_ptr->node[num].sent++;
                        send_byte(control, num, '1');
                        memset(&control->shared_ptr->node[num].to_send, 0, sizeof(control->shared_ptr->node[num].to_send));
						//signal the next node
                        SIGNAL_SEM(control, TO_SEND(num));
					//if sending, send the packet
                    } else if (sending) {
                        send_pkt(control, num);
					//if receiving, get the packet
                    } else {
                        if (getting==1) {
                            rcv_pkt.data[curr] = byte;
                        }
						//send the byte
                        send_byte(control, num, byte);
						//if the current byte is the last byte
                        if (++curr == len) {
							//if receiving, increment the received count
                            if (getting==1) {
                                control->shared_ptr->node[num].received++;
                                memset(&rcv_pkt, 0, sizeof(rcv_pkt));
                            }
							//clear the flags
                            curr = getting = 0;
                        }
                    }
                    break;
            }
        }
		//signal the next node
        SIGNAL_SEM(control, CRIT);
		//signal the next node
        SIGNAL_SEM(control, EMPTY(num));
		//signal the next node
        SIGNAL_SEM(control, FILLED(next_node));
    }
	//exit the thread
	free(arguments);
	return NULL;
}

/*
* This function sends a data packet followed by the token, one byte each
* time it is called.
*/
void
send_pkt(control, num)
	struct TokenRingData *control;
	int num;
{
	static int sndpos, sndlen;
	//switch for the state
	switch (control->snd_state) {
	//if the state is token flag
	case TOKEN_FLAG:
		//send the byte
		send_byte(control, num, '0');
		//set the state to TO
		control->snd_state = TO;
		break;
	//if the state is TO
	case TO:
		//send the byte
		send_byte(control, num, control->shared_ptr->node[num].to_send.to);
		//set the state to FROM
		control->snd_state = FROM;
		break;
	//if the state is FROM
	case FROM:
		//send the byte
		send_byte(control, num, control->shared_ptr->node[num].to_send.from);
		//set the state to LEN
		control->snd_state = LEN;
		
		break;

	case LEN:
		//send the byte
		send_byte(control, num, control->shared_ptr->node[num].to_send.length);
		//set the state to DATA
		control->snd_state = DATA;
		//set the length and position
		sndlen = control->shared_ptr->node[num].to_send.length;
		sndpos = 0;
		break;

	case DATA:
		//send the byte
		send_byte(control, num, control->shared_ptr->node[num].to_send.data[sndpos]);
		//if the current byte is not the last byte
		if (sndpos < sndlen - 1) {
			//increment the position and set the state to DATA
			sndpos++;
			control->snd_state = DATA;
		} else {
			//set the state to DONE
			control->snd_state = DONE;
		}
		break;
	//if the state is DONE
	case DONE:
		//clear the packet and flags
		control->shared_ptr->node[num].to_send.token_flag = '0';
		//send the byte
		send_byte(control, num, '1');
		control->shared_ptr->node[num].sent ++;
		sndlen = 0;
		break;	
	};
}

/*
* Send a byte to the next node on the ring. MUST be in crtical section.
*/
void
send_byte(control, num, byte)
	struct TokenRingData *control;
	int num;
	unsigned byte;
{
	//get the next node
	int next_node = (num + 1) % N_NODES;
	//send the byte at the next node
	control->shared_ptr->node[next_node].data_xfer = byte;
}

/*
* Receive a byte for this node.
*/
unsigned char
rcv_byte(control, num)
	struct TokenRingData *control;
	int num;
{
	unsigned char byte;
	//get the byte from the node
	byte = (control->shared_ptr)->node[num].data_xfer;
	//return the byte
	return byte;
}