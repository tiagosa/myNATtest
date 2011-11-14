#include <nice/nice.h>
#include <stdio.h>
#include <stdlib.h>
#include <sting.h>


NiceAgent *agent;
guint stream_id;
GSList *localCands,*remoteCands;
GMainLoop *loop;
guint myGlobalNiceComponentState;
GSList *rcands;
char agentDistinct;

void print_candidate_info(NiceCandidate *cand)
{
      gchar str_ip[256];

      switch (cand->type)
        {
        case NICE_CANDIDATE_TYPE_HOST:
          puts ("  NICE_CANDIDATE_TYPE_HOST");
          break;
        case NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE:
          puts ("  NICE_CANDIDATE_TYPE_SERVER_REFLEXIVE");
          break;
        case NICE_CANDIDATE_TYPE_PEER_REFLEXIVE:
          puts ("  NICE_CANDIDATE_TYPE_PEER_REFLEXIVE");
          break;
        case NICE_CANDIDATE_TYPE_RELAYED:
          puts ("  NICE_CANDIDATE_TYPE_RELAYED");
          break;
        default:
          puts ("  UNKNOWN CANDIDATE!");
          break;
        }
      switch (cand->transport)
        {
        case NICE_CANDIDATE_TRANSPORT_UDP:
          puts ("  NICE_CANDIDATE_TRANSPORT_UDP");
          break;
        default:
          puts ("UNKNOWN TRANSPORT");
          break;
        }

      nice_address_to_string (&(cand->addr), str_ip);
      printf ("  ADDRESS: %s\n", str_ip);
      nice_address_to_string (&(cand->base_addr), str_ip);
      printf ("  BASE ADDRESS: %s\n", str_ip);
      printf ("  PRIORITY: %d\n", cand->priority);
}

GSList * readCandsFile(char *fileName){
//Test reading candidates from file:
   puts("READING CANDIDATES FROM FILE");
   FILE *file = fopen(fileName, "rb"); /* read from the file in binary mode */
   int readNumCands;

   GSList *i=NULL;
   if ( file != NULL )
   {
     fread(&readNumCands,sizeof(int),1,file);
     printf("READ %d CANDIDATES FROM %s.\n",readNumCands,fileName);

     int n;
     for (n = 0; n<readNumCands; n++)
     {
       NiceCandidate *cand;
       fread(&cand,sizeof(cand),1,file);
       i = g_slist_append( i,cand);
     }
     fclose(file);
   }
   else
   {
     puts("ERROR OPENING FILE");
   }

   return i;
}

int writeCandsFile(GSList *cands,char *fileName){
// Write local candidates to file:
   FILE *file = fopen(fileName, "ab"); /* Use "a" to append to the file .binary mode */
   GSList *i;


   if ( file != NULL )
   {
     //write localCands length

     int len= g_slist_length(cands);
     printf("WRITING NUMBER OF CANDS=%d TO %s\n",len,fileName);
     fwrite(&len, sizeof(int), 1, file);

   
     for (i = cands; i; i = i->next)
     {
       NiceCandidate *cand = (NiceCandidate *) i->data;
       fwrite(&cand, sizeof cand, 1, file);
     }
     fflush(stdout);
     fclose(file);
     return 0;
   }
   else
   {
     puts("ERROR OPENING FILE");
     return 1;
   }

}


void
cb_nice_recv (NiceAgent * agent, guint stream_id, guint component_id,
	      guint len, gchar * buf, gpointer user_data)
{
  printf ("cb_nice_recv\n");
  printf ("RECEIVED STRING: %s\n",buf);
  
}


void
cb_candidate_gathering_done (void)
{
  printf ("ENTERING cb_candidate_gathering_done\n");
  char *fileName;
  localCands = nice_agent_get_local_candidates (agent, stream_id, 1);

  // tiago: print local candidates
  GSList *i;
  int n=0;
  for (i = localCands; i; i = i->next)
    {
      NiceCandidate *cand = (NiceCandidate *) i->data;
      printf("CANDIDADE NUMBER %d:\n",n);
      print_candidate_info(cand); 
      n++;
    }
  // Write local candidates to file:
   puts("WRITING CANDIDATES TO FILE");
   if (agentDistinct=='l')
   {
      fileName = "leftCands.bin";
   }
   else
   {
      fileName = "rightCands.bin";
   }
   
   writeCandsFile(localCands, fileName);


   puts("Wait until you change candidates");
   fflush(stdin);
   fflush(stdout);
   getchar();
   getchar();
   
   if (agentDistinct=='l')
   {
      fileName = "rightCands.bin";
   }
   else
   {
      fileName = "leftCands.bin";
   }

   GSList *remoteCandidates=NULL;
   remoteCandidates=readCandsFile(fileName);
   g_assert(remoteCandidates);


   gchar *frag, *pwd;
  nice_agent_get_local_credentials (agent, stream_id, &frag, &pwd);
  printf("\nLOCAL CREDENTIALS:\nUSER: %s\nPWD: %s\n",frag,pwd);
  //local_credentials are locally generated. should be shared with the remote agent. Should not be the same.

  printf("GETTING REMOTE CREDENTIALS:\n");
  printf("ENTER USER: ");
  char rUser[80], rPWD[80];
  fgets(rUser, 10, stdin);
  printf("ENTER PWD: ");
  fgets(rPWD, 10, stdin);
  fflush(stdin);
  nice_agent_set_remote_credentials (agent, stream_id, rUser, rPWD);
  //need to change the credentials with the remote agent
   

  nice_agent_set_remote_candidates (agent, stream_id, 1, rcands);

  //check remote candidates were correctly set
  GSList *tempCandidateList = NULL;
  tempCandidateList = nice_agent_get_remote_candidates(agent, stream_id, 1);

  puts("PRINTING REMOTE CANDIDATES LIST");
  GSList *iRemote;
  int nR=0;
  for (iRemote = tempCandidateList; iRemote; iRemote = iRemote->next)
    {
      NiceCandidate *cand = (NiceCandidate *) iRemote->data;
      printf("CANDIDATE NUMBER %d:\n",n);
      print_candidate_info(cand);
      nR++;
    }


  
}


void cb_component_state_changed(NiceAgent *agent, guint stream_id, guint component_id, guint state, gpointer data)
{
	char *NiceComponentStateString[] = {
		"NICE_COMPONENT_STATE_DISCONNECTED",
		"NICE_COMPONENT_STATE_GATHERING",
		"NICE_COMPONENT_STATE_CONNECTING",
		"NICE_COMPONENT_STATE_CONNECTED",
		"NICE_COMPONENT_STATE_READY",
		"NICE_COMPONENT_STATE_FAILED",
		"NICE_COMPONENT_STATE_LAST" };
	
	myGlobalNiceComponentState = state; //imposta var globale

	puts("CB_COMPONENT STATE CHANGED");
	printf("%d : %s\n", state, NiceComponentStateString[state]);

	switch(state)
	{
		case NICE_COMPONENT_STATE_READY:
			nice_agent_send(agent, stream_id, NICE_COMPONENT_TYPE_RTP, strlen("Ola"),"Ola");
			break;
		case NICE_COMPONENT_STATE_FAILED: //fechar loop
			if(g_main_loop_is_running (loop)) {
				puts("Stopping the loop. :(\n");
				g_main_loop_quit(loop);
			}
			else { g_error ("Alert: No loop! :("); }
			break;
	}
	return;
}





void
cb_new_selected_pair (void)
{
  printf ("cb_new_selected_pair\n");
}


int
main (int argc, char *argv[])
{
  if (argc==2) //separate two different agents. Useful to select candidates?
  {
	agentDistinct= (char) argv[1][0];
  }
  else
	agentDistinct='l';


  g_type_init ();
  g_thread_init (NULL);

  loop = g_main_loop_new (NULL, FALSE);

  nice_debug_enable (TRUE);

  // Create a nice agent
  agent =
    nice_agent_new_reliable (g_main_loop_get_context (loop),
			     NICE_COMPATIBILITY_RFC5245);

  //added by tiago: set relay info
  if (nice_agent_set_relay_info
      (agent, stream_id, 1, "66.228.45.110", 3478, "tiagosa@di.uminho.pt",
       "jpsderto", NICE_RELAY_TYPE_TURN_UDP))
    {
      printf ("RELAY SERVER ACCEPTED");
    }


  //Alternative: "127.0.0.1", 3478
  //NUMB server
  g_object_set (G_OBJECT (agent), "stun-server", "66.228.45.110", "stun-server-port", 3478, NULL);

  // Connect the signals
  g_signal_connect (G_OBJECT (agent), "candidate-gathering-done",
		    G_CALLBACK (cb_candidate_gathering_done), GUINT_TO_POINTER(1));
  g_signal_connect (G_OBJECT (agent), "component-state-changed",
		    G_CALLBACK (cb_component_state_changed), GUINT_TO_POINTER(1));
  g_signal_connect (G_OBJECT (agent), "new-selected-pair",
		    G_CALLBACK (cb_new_selected_pair), GUINT_TO_POINTER(1));

  // Create a new stream with one component and start gathering candidates
  stream_id = nice_agent_add_stream (agent, 1);

  // Attach to the component to receive the data

  nice_agent_attach_recv (agent, stream_id, 1, g_main_loop_get_context (loop),
			  cb_nice_recv, NULL);

  printf ("START nice_agent_gather_candidates");
  nice_agent_gather_candidates (agent, stream_id);

  g_main_loop_run (loop);

  // Destroy the object
  g_object_unref (agent);

  return 0;
}
