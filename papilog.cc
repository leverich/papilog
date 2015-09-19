#include <papi.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "cmdline.h"
#include "log.h"

gengetopt_args_info args;

#define MAX_EVENTS 16

int event_set = PAPI_NULL;

int num_events = 0;
int events[MAX_EVENTS];
long long values[MAX_EVENTS];

void sigalrm_handler(int signum) {
  PAPI_read(event_set, values);

  if (args.reset_given) PAPI_reset(event_set);

  if (args.timestamp_given) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("%ld.%06ld ", tv.tv_sec, tv.tv_usec);
  }

  for (int i = 0; i < num_events; i++)
    printf("%s%lld", (i == 0 ? "" : " " ), values[i]);
  printf("\n");

  fflush(stdout);
}

int main(int argc, char **argv) {
  if (cmdline_parser(argc, argv, &args) != 0) exit(-1);

  for (unsigned int i = 0; i < args.verbose_given; i++)
    log_level = (log_level_t) ((int) log_level -1);

  if (args.pid_arg < 1) DIE("--pid must be > 0");
  if (kill(args.pid_arg, 0) != 0) DIE("PID %d does not exist.", args.pid_arg);

  if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
    DIE("PAPI_library_init() failed");
  if (PAPI_create_eventset(&event_set) != PAPI_OK)
    DIE("PAPI_create_eventset() failed");

  if (args.event_given) { // Parse command-line options to register events.
    int event_code;

    for (unsigned int i = 0; i < args.event_given; i++) {
      if (PAPI_event_name_to_code(args.event_arg[i], &event_code) != PAPI_OK)
        DIE("Invalid PAPI event %s", args.event_arg[i]);

      if (PAPI_add_event(event_set, event_code) != PAPI_OK)
        DIE("PAPI_add_event(%s) failed", args.event_arg[i]);

      events[num_events++] = event_code;
    }
  } else {
    events[num_events++] = PAPI_TOT_CYC;
    events[num_events++] = PAPI_TOT_INS;

    if (PAPI_add_event(event_set, PAPI_TOT_CYC) != PAPI_OK)
      DIE("PAPI_add_event(PAPI_TOT_CYC) failed");

    if (PAPI_add_event(event_set, PAPI_TOT_INS) != PAPI_OK)
      DIE("PAPI_add_event(PAPI_TOT_INS) failed");
  }

  printf("#");

  if (args.timestamp_given) printf("time ");

  for (int i = 0; i < num_events; i++) {
    char event_label[20];

    PAPI_event_code_to_name(events[i], event_label);
    printf("%s%s", (i == 0 ? "" : " " ), event_label);
  }
  printf("\n");

  if (PAPI_attach(event_set, args.pid_arg) != PAPI_OK)
    DIE("PAPI_attach(pid = %d) failed", args.pid_arg);

  if (PAPI_start(event_set) != PAPI_OK)
    DIE("PAPI_start() failed");

  struct itimerval itv;

  itv.it_interval.tv_sec = args.interval_arg;
  itv.it_interval.tv_usec = 0;
  itv.it_value.tv_sec = args.interval_arg;
  itv.it_value.tv_usec = 0;

  signal(SIGALRM, sigalrm_handler);

  if (setitimer(ITIMER_REAL, &itv, NULL) == -1) DIE("setitimer() failed");

  while(1) sleep(86400);
}
