#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define MAX_KEY_LEN    32
#define MAX_VAL_LEN    64
#define MAX_CMD_LEN    128
#define MAX_RESP_LEN   128
#define BUFFER_SIZE    4096
#define TIMEOUT_SEC    2
#define PROGRESS_TIMEOUT 5

// 引擎类型
typedef enum {
    ENGINE_ARRAY,
    ENGINE_RBTREE,
    ENGINE_HASH
} engine_type_t;

const char* engine_prefix[] = {
    "",     // array 使用 SET/GET
    "R",    // rbtree 使用 RSET/RGET
    "H"     // hash 使用 HSET/HGET
};

typedef struct {
    char* host;
    int port;
    int connections;
    long total_requests;
    double get_ratio;
    int duration;
    engine_type_t engine;
} config_t;

config_t config = {
    .host = "127.0.0.1",
    .port = 9096,
    .connections = 64,
    .total_requests = 100000,
    .get_ratio = 1.0,
    .duration = 0,
    .engine = ENGINE_ARRAY
};

typedef struct {
    int sockfd;
    long requests_done;
    long errors;
    double total_latency;
} thread_stat_t;

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int stop_flag = 0;
long long global_requests = 0;
long long global_errors = 0;
double global_latency = 0.0;
struct timeval test_start, test_end;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        stop_flag = 1;
    }
}

void parse_args(int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "h:p:c:n:r:t:e:")) != -1) {
        switch (opt) {
        case 'h': config.host = optarg; break;
        case 'p': config.port = atoi(optarg); break;
        case 'c': config.connections = atoi(optarg); break;
        case 'n': config.total_requests = atol(optarg); break;
        case 'r': config.get_ratio = atof(optarg); break;
        case 't': config.duration = atoi(optarg); break;
        case 'e':
            if (strcmp(optarg, "array") == 0) config.engine = ENGINE_ARRAY;
            else if (strcmp(optarg, "rbtree") == 0) config.engine = ENGINE_RBTREE;
            else if (strcmp(optarg, "hash") == 0) config.engine = ENGINE_HASH;
            else {
                fprintf(stderr, "Engine must be 'array', 'rbtree', or 'hash'\n");
                exit(1);
            }
            break;
        default:
            fprintf(stderr, "Usage: %s [-h host] [-p port] [-c connections] [-n requests] [-r get_ratio] [-t seconds] [-e array|rbtree|hash]\n", argv[0]);
            exit(1);
        }
    }
}

void set_socket_timeout(int sockfd) {
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int connect_server(const char* host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;
    set_socket_timeout(sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

void rand_str(char* buf, int len) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < len - 1; i++) {
        buf[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buf[len - 1] = '\0';
}

int do_request(int sockfd, const char* cmd, char* resp, int resp_len) {
    int len = strlen(cmd);
    if (send(sockfd, cmd, len, 0) != len) return -1;

    int total = 0;
    while (total < resp_len - 1) {
        ssize_t n = recv(sockfd, resp + total, 1, 0);
        if (n <= 0) return -1;
        total += n;
        if (total >= 2 && resp[total - 2] == '\r' && resp[total - 1] == '\n') break;
    }
    resp[total] = '\0';
    return 0;
}

void* worker_thread(void* arg) {
    thread_stat_t* stat = (thread_stat_t*)arg;
    int sockfd = stat->sockfd;
    char key[MAX_KEY_LEN];
    char val[MAX_VAL_LEN];
    char cmd[MAX_CMD_LEN];
    char resp[MAX_RESP_LEN];
    long requests = 0, errors = 0;
    double latency_sum = 0.0;
    int reconnect_failures = 0;
    const int max_reconnect_failures = 5;
    const char* prefix = engine_prefix[config.engine];

    while (!stop_flag) {
        rand_str(key, 8);
        int is_get = ((double)rand() / RAND_MAX) < config.get_ratio;

        if (is_get) {
            snprintf(cmd, sizeof(cmd), "%sGET %s\r\n", prefix, key);
        }
        else {
            rand_str(val, 16);
            snprintf(cmd, sizeof(cmd), "%sSET %s %s\r\n", prefix, key, val);
        }

        struct timeval t1, t2;
        gettimeofday(&t1, NULL);

        int ret = do_request(sockfd, cmd, resp, sizeof(resp));
        if (ret < 0) {
            errors++;
            close(sockfd);
            sockfd = connect_server(config.host, config.port);
            if (sockfd < 0) {
                reconnect_failures++;
                if (reconnect_failures >= max_reconnect_failures) break;
                usleep(100000);
                continue;
            }
            stat->sockfd = sockfd;
            reconnect_failures = 0;
            continue;
        }

        gettimeofday(&t2, NULL);
        long elapsed = (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
        latency_sum += elapsed;
        requests++;

        if (strstr(resp, "SUCCESS") == NULL && strstr(resp, "NO EXIST") == NULL) {
            errors++;
        }

        pthread_mutex_lock(&global_mutex);
        global_requests++;
        global_errors += (ret < 0) ? 1 : 0;
        global_latency += elapsed;
        pthread_mutex_unlock(&global_mutex);

        if (config.duration == 0 && global_requests >= config.total_requests) {
            break;
        }
    }

    stat->requests_done = requests;
    stat->errors = errors;
    stat->total_latency = latency_sum;
    close(sockfd);
    return NULL;
}

int main(int argc, char** argv) {
    parse_args(argc, argv);
    signal(SIGINT, handle_signal);
    srand(time(NULL));

    printf("Benchmark configuration:\n");
    printf("  Host: %s\n", config.host);
    printf("  Port: %d\n", config.port);
    printf("  Connections: %d\n", config.connections);
    printf("  Total requests: %ld\n", config.total_requests);
    printf("  GET ratio: %.2f\n", config.get_ratio);
    printf("  Engine: %s\n", config.engine == ENGINE_ARRAY ? "array" :
        (config.engine == ENGINE_RBTREE ? "rbtree" : "hash"));
    if (config.duration > 0) printf("  Duration: %d seconds\n", config.duration);

    pthread_t* threads = malloc(config.connections * sizeof(pthread_t));
    thread_stat_t* stats = malloc(config.connections * sizeof(thread_stat_t));

    gettimeofday(&test_start, NULL);

    for (int i = 0; i < config.connections; i++) {
        int sock = connect_server(config.host, config.port);
        if (sock < 0) {
            fprintf(stderr, "Failed to connect to server (attempt %d).\n", i + 1);
        }
        stats[i].sockfd = sock;
        stats[i].requests_done = 0;
        stats[i].errors = 0;
        stats[i].total_latency = 0.0;
        pthread_create(&threads[i], NULL, worker_thread, &stats[i]);
    }

    if (config.duration > 0) {
        for (int i = 0; i < config.duration; i++) {
            if (stop_flag) break;
            sleep(1);
        }
        stop_flag = 1;
    }
    else {
        long long last_requests = 0;
        int stall_seconds = 0;
        while (global_requests < config.total_requests && !stop_flag) {
            sleep(1);
            if (global_requests == last_requests) {
                stall_seconds++;
                if (stall_seconds >= PROGRESS_TIMEOUT) {
                    printf("\nNo progress for %d seconds, exiting early.\n", PROGRESS_TIMEOUT);
                    stop_flag = 1;
                    break;
                }
            }
            else {
                stall_seconds = 0;
                last_requests = global_requests;
            }
        }
        if (!stop_flag && global_requests >= config.total_requests) {
            stop_flag = 1;
        }
    }

    for (int i = 0; i < config.connections; i++) {
        pthread_join(threads[i], NULL);
    }

    gettimeofday(&test_end, NULL);

    double elapsed = (test_end.tv_sec - test_start.tv_sec) +
        (test_end.tv_usec - test_start.tv_usec) / 1000000.0;

    long long total_req = global_requests;
    long long total_err = global_errors;
    double avg_latency = (total_req > 0) ? (global_latency / total_req) : 0.0;

    printf("\n--- Results ---\n");
    printf("Total requests:    %lld\n", total_req);
    printf("Errors:            %lld (%.2f%%)\n", total_err, total_err * 100.0 / (total_req ? total_req : 1));
    printf("Time elapsed:      %.2f s\n", elapsed);
    printf("QPS:               %.2f\n", total_req / elapsed);
    printf("Average latency:   %.2f us\n", avg_latency);

    free(threads);
    free(stats);
    return 0;
}