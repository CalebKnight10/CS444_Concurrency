// Caleb Knight
// CS444
// Classic Concurrency Problems

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/wait.h>

#define RESOURCE_COUNT 3
#define RESOURCE_TOTAL ((RESOURCE_COUNT * (RESOURCE_COUNT-1)) / 2)  // 0 + 1 + 2 = 3 total
#define INSTRUCTIONS "Instructions:\n\n\
First argument:\n\
  -proc \n\
Second argument:\n\
  -p: run the producer/consumer problem\n\
    -n {N}: number of producers (required if using -p)\n\
    -c {C}: number of consumers (required if using -p)\n\
  -d: run the dining philosopher's problem\n\
  -b: run the potion brewers problem\n\
Examples:\n\
  ./Knight_Concurrency -proc -p -n 3 -c 2\n\
  ./Knight_Concurrency -proc -d\n\
  ./Knight_Concurrency -proc -b\n"

extern int errno;

// init and destroy named semaphores
void open_sem(sem_t **semaphore, char *name, int init_value) {
    *semaphore = sem_open("semaphore", O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (*semaphore == SEM_FAILED) {
        fprintf(stderr, "Error in sem_open %s: %s\n", name, strerror(errno));
    }
}

void destroy_sem(sem_t *semaphore, char *name) {
    if(sem_close(semaphore) == -1) {
        fprintf(stderr, "Error in sem_close %s: %s\n", name, strerror(errno));
    }
    if(sem_unlink(name) == -1) {
        fprintf(stderr, "Error in sem_unlink %s: %s\n", name, strerror(errno));
    }
}


/**********************************************************Producer/Consumer************************************************/
// Taken and modified from example on Canvas
struct pc_args {
    int id;
    int pipefds;
    int p_loop;
    int c_loop;
};

void initalize(struct pc_args *args, int id, int p_loop, int c_loop, int fd) {
    args->id = id;
    args->p_loop = p_loop;
    args->c_loop = c_loop;
    args->pipefds = fd;
}

void consumer(struct pc_args *args) {
    int value;
    for (int i = 0; i < args->c_loop; i++) {
        sleep(rand() % 3);  // sleep between 0 and 2 seconds
        read(args->pipefds, &value, sizeof(int));  // write value into the pipe
        printf("consumer %d got:    %d\n", args->id, value);
        fflush(stdout);  // output gets flushed right after
    }
    exit(EXIT_SUCCESS);
}

void producer(struct pc_args *args) {
    int value;
    for (int i = 0; i < args->p_loop; i++) {
        sleep(rand() % 3);
        value = (args->id * 1000) + i + 1;  // 2010 will be the 10th item made by process #2
        write(args->pipefds, (void *)&value, sizeof(int));
        printf("producer %d put:  %d\n", args->id, value);
        fflush(stdout);
    }
    exit(EXIT_SUCCESS);
}

int run(int producer_num, int consumer_num) {
    printf("Producer/Consumer problem with %d producers, %d consumers...\n\n", producer_num, consumer_num);
    int fds[2]; // 0 read 1 write
    if (pipe(fds) == -1) {
        printf("Call to pipe() failed\n");
        exit(EXIT_FAILURE);
    }

    pid_t producers[producer_num];
    pid_t consumers[consumer_num];
    struct pc_args args;
    // creatine all processes
    for (int i = 0; i < producer_num; i++) {
        initalize(&args, i + 1, consumer_num * 3, producer_num * 3, fds[1]);
        if((producers[i] = fork()) == 0) producer(&args);
    }

    for (int i = 0; i < consumer_num; i++) {
        initalize(&args, i + 1, consumer_num * 3, producer_num * 3, fds[0]);
        if((consumers[i] = fork()) == 0) consumer(&args);
    }

    // wait for all processes
    int status;
    for (int i = 0; i < producer_num; i++) {
      waitpid(producers[i], &status, 0);
    }

    for (int i = 0; i < consumer_num; i++) {
      waitpid(consumers[i], &status, 0);
    }
    return EXIT_SUCCESS;
}


/***************************************************Dining Philosopher***********************************/
void init_fork_sems(sem_t *forks[], char *fork_sems[]) {
    for(int i = 0; i < 5; i++)  open_sem(&forks[i], fork_sems[i], 1);
}

void destroy_fork_sems(sem_t *forks[], char *fork_sems[]) {
    for(int i = 0; i < 5; i++)  destroy_sem(forks[i], fork_sems[i]);
}

// functions to acquire and put down the fork semaphores on either side of a philosopher
void get_forks(int p, sem_t *left, sem_t *right) {
    // to prevent deadlock - need at least one philosopher to acquire forks in different order
    printf("Philosopher %d is getting forks...\n", p);
    fflush(stdout);
    if(p == 4) {
        sem_wait(right);
        sem_wait(left);
    } else {
        sem_wait(left);
        sem_wait(right);
    }
}

void put_forks(int p, sem_t *left, sem_t *right) {
    sem_post(left);
    sem_post(right);
}

// thinkin and eatin are both I/O ops simulating by sleeping
void thinkin(int p) {
    printf("Philosopher %d is thinkin\n", p);
    fflush(stdout);
    sleep((rand() % 20) + 1);  // thinkining will take a random amount of time in the range of 1-20 seconds.
}
void eatin(int p) {
    printf("Philosopher %d is eatin\n", p);
    fflush(stdout);
    sleep((rand() % 8) + 2);  // eatining will take a random amount of time in the range of 2-9 seconds.
}

// functions to get the left / right fork from a given philosopher
int left(int p) { 
  return p; 
}

int right(int p) { 
  return (p + 1) % 5; 
}

// function to run philosopher problem
int philosopher_work(int id, sem_t *left, sem_t *right) {
    for(int i = 0; i < 5; i++) {
        thinkin(id);
        get_forks(id, left, right);
        eatin(id);
        put_forks(id, left, right);
    }
    return EXIT_SUCCESS;
}

int philosopher() {
    printf("Dining Philosopher's problem... Everyone eatins 5 times before termination.\n\n");
    sem_t *forks[5];  // semaphores that represent each of the five forks
    char *fork_sems[5] = {"/s0", "/s1", "/s2", "/s3", "/s4"}; // semaphore names
    init_fork_sems(forks, fork_sems);
    // creatine all the processes
    pid_t philosophers[5];
    for (int i = 0; i < 5; i++)
        if((philosophers[i] = fork()) == 0)
            return philosopher_work(i, forks[left(i)], forks[right(i)]);
    // wait for all the processes
    int status;
    for (int i = 0; i < 5; i++)
        waitpid(philosophers[i], &status, 0);
    destroy_fork_sems(forks, fork_sems);
    return EXIT_SUCCESS;
}


/*****************************************************Brew Master************************************/

struct brewmaster_sems {
    sem_t *resource_sems[RESOURCE_COUNT];
    char *resource_sem_names[RESOURCE_COUNT];
    sem_t *agent_sem;
    char *agent_sem_name;    
    sem_t *resource_state_mutex;
    char *resource_state_mutex_name;    
    sem_t *brewers_sems[RESOURCE_COUNT];
    char *brewers_sems_names[RESOURCE_COUNT];
} sems = {
        .resource_sem_names = {"/unicorn_horns", "/bezoars", "/mistletoe_berries"},
        .agent_sem_name = "/agent_brewmaster",
        .resource_state_mutex_name = "/pusher_state",
        .brewers_sems_names = {"/mistletoe_berries_brewer", "/unicorn_horns_brewer", "/bezoars_brewer"}
};;

struct ResourceState {
    int count; 
    int total; 
} resourceState = {0, 0};

// functions to initialize and destroy the semaphores
void init_brewmaster_sems(struct brewmaster_sems *sems) {
    open_sem(&sems->agent_sem, sems->agent_sem_name, 1);
    open_sem(&sems->resource_state_mutex, sems->resource_state_mutex_name, 1);
    for(int i = 0; i < RESOURCE_COUNT; i++) {
        open_sem(&sems->resource_sems[i], sems->resource_sem_names[i], 0);
        open_sem(&sems->brewers_sems[i], sems->brewers_sems_names[i], 0);
    }
}
void destroy_brewmaster_sems(struct brewmaster_sems *sems) {
    destroy_sem(sems->agent_sem, sems->agent_sem_name);
    destroy_sem(sems->resource_state_mutex, sems->resource_state_mutex_name);
    for(int i = 0; i < RESOURCE_COUNT; i++) {
        destroy_sem(sems->resource_sems[i], sems->resource_sem_names[i]);
        destroy_sem(sems->brewers_sems[i], sems->brewers_sems_names[i]);
    }
}

// produces all resource, except the one that it is explicitly missing
int agent_work(int missing_resource) {
    for(int i = 0; i < 5; i++) { // produce 5 times
        sem_wait(sems.agent_sem);
        usleep(150000);  // sleep .15 seconds
        for(int j = 0; j < RESOURCE_COUNT; j++) {
            // e.g. if this thread is 0 = bezoars, this threads skips 0 but produces 1 and 2
            if(j != missing_resource) {
                printf("Agent produced:     %s\n", sems.resource_sem_names[j]+1);
                fflush(stdout);
                sem_post(sems.resource_sems[j]);
            }
        }
    }
    return EXIT_SUCCESS;
}

// wakes up the appropriate brewer based on what resources the agents produce
int pusher_work(int assigned_resource, int pipeFD) {
    // wait for this resource to be produced by the agent
    // then push the resource into the pipe
    for(int i = 0; i < (10 * (RESOURCE_COUNT - 1)); i++) {
        sem_wait(sems.resource_sems[assigned_resource]);
        write(pipeFD, (void *)&assigned_resource, sizeof(int));
    }
    return EXIT_SUCCESS;
}

// woken up by pusher
// thenproduces potion
// then wakes up agent for the next batch of resources
int brewer_work(int assigned_resource) {
    for(int i = 0; i < 10; i++) {
        sem_wait(sems.brewers_sems[assigned_resource]);
        printf("Potion by brewer with:   %s\n", sems.resource_sem_names[assigned_resource]+1);
        fflush(stdout);
        sem_post(sems.agent_sem);
    }
    return EXIT_SUCCESS;
}

int agent(int missing_resource) {
  return agent_work(missing_resource);
}

int brewer(int assigned_resource) {
  return brewer_work(assigned_resource);
}

int pusher(int assigned_resource, int pipefds) {
    // wait for this resource to be produced by the agent, then push the resource into the pipe
    for(int i = 0; i < (10 * (RESOURCE_COUNT - 1)); i++) {
        sem_wait(sems.resource_sems[assigned_resource]);
        write(pipefds, (void *)&assigned_resource, sizeof(int));
    }
    return EXIT_SUCCESS;
}

int resources(int pipefds) {
    // read the resources in from the pipe
    // if a pair is found
    // then signal the brewing() w/ missing resource
    int count = 0;
    int total = 0;
    int produced_resource = 0;
    for(int i = 0; i < (10 * (RESOURCE_COUNT - 1) * RESOURCE_COUNT); i++) {
        read(pipefds, &produced_resource, sizeof(int));
        count++;
        total += produced_resource;
        // if this is second resource
        // then wake up the brewer with the missing resource and reset state to 0
        if(count == (RESOURCE_COUNT - 1)) {
            sem_post(sems.brewers_sems[RESOURCE_TOTAL - total]);
            count = 0;
            total = 0;
        }
    }
    return EXIT_SUCCESS;
}

int brew_master() {
    printf("Brew master problem... Producing 5 types of each.\n\n");
    init_brewmaster_sems(&sems);
    // creatine pipe which is used to transfer resource from the pushers to the resources
    int fds[2]; 
    if (pipe(fds) == -1) {
        printf("Call to pipe() failed\n");
        exit(EXIT_FAILURE);
    }
    // creatine all the processes
    pid_t agents[RESOURCE_COUNT];  // agent to produce resources
    pid_t pushers[RESOURCE_COUNT];  // pusher to interface between agent and brewers
    pid_t brewers[RESOURCE_COUNT];  // brewer to produce potions
    pid_t resource;
    for (int i = 0; i < RESOURCE_COUNT; i++) {
        if((agents[i] = fork()) == 0) return agent(i);
        if((pushers[i] = fork()) == 0) return pusher(i, fds[1]);
        if((brewers[i] = fork()) == 0) return brewer(i);
    }
    if((resource = fork()) == 0) return resources(fds[0]);
    // wait for all processes
    int status;
    for (int i = 0; i < RESOURCE_COUNT; i++) {
        waitpid(agents[i], &status, 0);
        waitpid(pushers[i], &status, 0);
        waitpid(brewers[i], &status, 0);
    }
    waitpid(resource, &status, 0);
    destroy_brewmaster_sems(&sems);
    return EXIT_SUCCESS;
}


/**********************************************************Instructions and Main***********************************************************/
int print_instructions() {
    printf("%s", INSTRUCTIONS);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    if (argc < 3)
        return print_instructions();
    if(strncmp(argv[2], "-p", 2) == 0) {
        // consumer producer problem
        if(argc < 7)
            return print_instructions();
        if(strncmp(argv[3], "-n", 2) != 0 || strncmp(argv[5], "-c", 2) != 0)
            return print_instructions();
        int producers = atoi(argv[4]);
        int consumers = atoi(argv[6]);
        return run(producers, consumers);
    } else if (strncmp(argv[2], "-d", 2) == 0) {
        // dining philosophers problem
        return philosopher();
    } else if (strncmp(argv[2], "-b", 2) == 0) {
        // brew master problem
        return brew_master();
    } else {
        return print_instructions();
    }
}