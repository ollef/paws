#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/pulseaudio.h>

void client_cb(pa_context* context,
                   pa_subscription_event_type_t t,
                   uint32_t idx,
                   void* userdata);
void context_subscribe_cb(pa_context* context, int success, void* userdata);
void context_state_cb(pa_context* context, void* userdata);
void input_sink_changed(pa_context* context,
                        const pa_sink_input_info* i, int eol, void* new);
void sink_change_client_info(pa_context* context,
                             const pa_client_info* i, int eol, void* new);

int verbose = 0;
#define vprintf(...) if(verbose) {printf(__VA_ARGS__);}

#define MAX_CLIENTS 64

/* The commands to run when another client appears to be starting or stopping
 * playback */
const char* resume_command = 0;
const char* pause_command = 0;
/* Clients to ignore */
const char* ignore_clients[MAX_CLIENTS];
int num_ignore_clients = 0;

/* Keep track of clients and how many input sinks they use */
struct {
  uint32_t idx;
  unsigned int input_sinks;
} clients[MAX_CLIENTS];

unsigned int active_clients;

void run_pause_command() {
  vprintf("Running pause command\n");
  if(pause_command) {
    system(pause_command);
  }
}

void run_resume_command() {
  vprintf("Running resume command\n");
  if(resume_command) {
    system(resume_command);
  }
}

/* Is the name in the ignore array? */
int is_ignore_client(const char* name) {
  for(int i = 0; i < num_ignore_clients; ++i) {
    if(strcmp(name, ignore_clients[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

/* Remove a client if it is in the client list */
void rem_client(uint32_t idx) {
  for(int i = 0; i < MAX_CLIENTS; ++i) {
    if(clients[i].idx == idx) {
      if(clients[i].input_sinks > 0) {
        --active_clients;
        if(active_clients == 0) {
          run_resume_command();
        }
      }
      clients[i].idx = PA_INVALID_INDEX;
      vprintf("Removed client %u (# active clients = %u)\n", idx, active_clients);
      return;
    }
  }
}

/* A client with index idx has started a new input sink */
void add_input_sink(uint32_t idx, const char* name) {
  if(is_ignore_client(name))
    return;
  for(int i = 0; i < MAX_CLIENTS; ++i) {
    if(clients[i].idx == idx) {
      ++clients[i].input_sinks;
      vprintf("Client %u (%s) added an input sink (# active clients = %u)\n", idx, name, active_clients);
      return;
    }
  }
  /* If we reach this point, the client has not been added yet */
  for(int i = 0; i < MAX_CLIENTS; ++i) {
    if(clients[i].idx == PA_INVALID_INDEX) {
      clients[i].idx = idx;
      clients[i].input_sinks = 1;
      ++active_clients;
      if(active_clients == 1) {
        run_pause_command();
      }
      vprintf("Client %u (%s) added its first input sink (# active clients = %u)\n", idx, name, active_clients);
      return;
    }
  }
}

/* A client with index idx has stopped an input sink */
void rem_input_sink(uint32_t idx, const char* name) {
  for(int i = 0; i < MAX_CLIENTS; ++i) {
    if(clients[i].idx == idx) {
      --clients[i].input_sinks;
      if(clients[i].input_sinks == 0) {
        clients[i].idx = PA_INVALID_INDEX;
        --active_clients;
        if(active_clients == 0) {
          run_resume_command();
        }
        vprintf("Client %u (%s) removed its last input sink (# active clients = %u)\n", idx, name, active_clients);
      } else {
        vprintf("Client %u (%s) removed an input sink (# active clients = %u)\n", idx, name, active_clients);
      }
      return;
    }
  }
  /* If we reach this point, the client is not handled by us */
}

int main(int argc, char** argv) {
  /* Fetch commandline arguments */
  int c;
  while((c = getopt(argc, argv, "vp:r:i:")) != -1) {
    switch(c) {
      case 'v':
        verbose = 1;
        break;
      case 'p':
        pause_command = optarg;
        break;
      case 'r':
        resume_command = optarg;
        break;
      case 'i':
        if(num_ignore_clients < MAX_CLIENTS) {
          ignore_clients[num_ignore_clients] = optarg;
          num_ignore_clients++;
        }
        break;
      default:
        printf("Usage: %s [-v] [-p pause-cmd] [-r resume-cmd] [-i ignore-client]\n", argv[0]);
        return EXIT_SUCCESS;
    }
  }
  /* Initialise clients array */
  for(int i = 0; i < MAX_CLIENTS; ++i) {
    clients[i].idx = PA_INVALID_INDEX;
  }

  /* Start asynchronous main loop */
  pa_mainloop*     mainloop     = pa_mainloop_new();
  pa_mainloop_api* mainloop_api = pa_mainloop_get_api(mainloop);
  pa_context*      context      = pa_context_new(mainloop_api, "paws");

  pa_context_connect(context, NULL, 0, NULL);
  pa_context_set_state_callback(context, context_state_cb, mainloop);

  pa_mainloop_run(mainloop, NULL);

  return 0;
}

void context_state_cb(pa_context* context, void* userdata) {
  pa_mainloop*       mainloop = userdata;
  pa_context_state_t state    = pa_context_get_state(context);
  switch(state) {
    case PA_CONTEXT_FAILED:
    case PA_CONTEXT_TERMINATED:
      pa_context_disconnect(context);
      pa_context_unref(context);
      pa_mainloop_free(mainloop);
      break;
    case PA_CONTEXT_READY:
      /* Ask if we can be allowed to subscribe to some events */
      pa_context_subscribe(context,
        PA_SUBSCRIPTION_MASK_CLIENT | PA_SUBSCRIPTION_MASK_SINK_INPUT,
        context_subscribe_cb, NULL);
      break;
    default:
      break;
  }
}

void context_subscribe_cb(pa_context* context, int success, void* userdata) {
  pa_context_set_subscribe_callback(context, client_cb, userdata);
}

void client_cb(pa_context* context,
                   pa_subscription_event_type_t t,
                   uint32_t idx,
                   void* userdata) {
  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
    case PA_SUBSCRIPTION_EVENT_CLIENT:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_NEW:
          break;
        case PA_SUBSCRIPTION_EVENT_REMOVE:
          rem_client(idx);
          break;
      }
      break;
    case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
      switch (t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) {
        case PA_SUBSCRIPTION_EVENT_NEW:
          pa_context_get_sink_input_info(context, idx, input_sink_changed, (void*) 1);
          break;
        case PA_SUBSCRIPTION_EVENT_REMOVE:
          pa_context_get_sink_input_info(context, idx, input_sink_changed, (void*) 0);
          break;
      }
      break;
  }
}

void input_sink_changed(pa_context* context,
                        const pa_sink_input_info* i, int eol, void* new) {
  if(!eol) {
    /* Get the client info of the sink that has changed */
    pa_context_get_client_info(context, i->client, sink_change_client_info, new);
  }
}

void sink_change_client_info(pa_context* context,
                             const pa_client_info* i, int eol, void* new) {
  if(!eol) {
    if((intptr_t) new) {
      add_input_sink(i->index, i->name);
    } else {
      rem_input_sink(i->index, i->name);
    }
  }
}
