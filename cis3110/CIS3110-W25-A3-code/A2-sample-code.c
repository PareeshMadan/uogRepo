*** ../CIS3110-W25-A2-code//./tokenRing_simulate.c	Sun Feb  2 13:19:39 2025
--- ././tokenRing_simulate.c	Sun Feb  2 13:19:39 2025
***************
--- 33,120 ----
  	 * If this is node #0, start the ball rolling by creating the
  	 * token.
  	 */
> 	if (num == 0)
> 		send_byte(control, num, '1');
  
  	while (not_done) {
		/** get a byte, then produce a byte */
> 		byte = rcv_byte(control, num);
  
  		/*
  		 * Handle the byte, based upon current state.
  		 */
  		switch (rcv_state) {

  		case TO:
> 			if (!sending) {
> 				if (byte == num)
> 					control->shared_ptr->node[num].received++;
> 				send_byte(control, num, byte);
> 			} else
> 				send_pkt(control, num);
> 			rcv_state = FROM;
  			break;

  		case TOKEN_FLAG:
> 			if (!sending) {
> 				if (byte == '1') {
> 					WAIT_SEM(control, CRIT);
> 					if (control->shared_ptr->node[num].to_send.length > 0) {
> 						SIGNAL_SEM(control, CRIT);
> 						control->snd_state = TOKEN_FLAG;
> 						sending = 1;
> 						control->shared_ptr->node[num].sent++;
> 						send_pkt(control, num);
> 					} else {
> 						if (control->shared_ptr->node[num].terminate)
> 							not_done = 0;
> 						SIGNAL_SEM(control, CRIT);
> 						send_byte(control, num, byte);
> 					}
> 					rcv_state = TOKEN_FLAG;
> 				} else {
> 					send_byte(control, num, byte);
> 					rcv_state = TO;
> 				}
> 			} else {
> 				if (byte != '0')
> 					panic("Multiple Tokens\n");
> 				send_pkt(control, num);
> 				rcv_state = TO;
> 			}
  			break;



***************
--- 127,166 ----

void
send_pkt(control, num)
	struct TokenRingData *control;
	int num;
{
	static int sndpos, sndlen;
  
  	switch (control->snd_state) {
  	case TOKEN_FLAG:
> 		send_byte(control, num, '0');
> 		control->snd_state = TO;
  		break;
  	case TO:
> 		send_byte(control, num, control->shared_ptr->node[num].to_send.to);
> 		control->snd_state = FROM;
  		break;


***************
--- 176,187 ----
  	int num;
  	unsigned byte;
  {
> 	int to;
> 
> 	to = (num + 1) % N_NODES;
> 	WAIT_SEM(control, EMPTY(to));
> 	control->shared_ptr->node[to].data_xfer = (unsigned char) byte;
> 	SIGNAL_SEM(control, FILLED(to));
  }
  
  /*
***************
--- 194,202 ----
  {
  	unsigned char byte;
  
> 	WAIT_SEM(control, FILLED(num));
> 	byte = control->shared_ptr->node[num].data_xfer;
> 	SIGNAL_SEM(control, EMPTY(num));
> 	return (byte);
  }
  
