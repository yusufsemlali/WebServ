#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define SERVER_HOST "127.0.0.1"
#define SERVER_PORT 1025
#define MAX_EVENTS 1024
#define BUFFER_SIZE 4096
#define GET_REQUEST "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
#define CGI_POST_DATA "num1=10&operation=add&num2=5"
#define CGI_REQUEST "POST /cgi/php/calc.php HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 28\r\nConnection: close\r\n\r\n" CGI_POST_DATA

/* Upload POST request with multipart form data */
#define UPLOAD_BOUNDARY "----WebServBoundary123456"
#define UPLOAD_BODY "------WebServBoundary123456\r\nContent-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\nContent-Type: text/plain\r\n\r\nThis is test content for stress testing uploads.\r\n------WebServBoundary123456--\r\n"
#define UPLOAD_REQUEST "POST /uploads HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary=----WebServBoundary123456\r\nContent-Length: 178\r\nConnection: close\r\n\r\n" UPLOAD_BODY

typedef struct {
    int fd;
    int state; /* 0=connecting, 1=sending, 2=receiving, 3=done */
    size_t bytes_sent;
    size_t bytes_received;
    time_t start_time;
} connection_t;

static volatile int running = 1;
static int total_requests = 0;
static int successful_requests = 0;
static int failed_requests = 0;
static int peak_concurrent = 0;
static time_t start_time;
static const char *request_template = GET_REQUEST;
static const char *request_mode = "GET";

void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_connection(int epoll_fd, connection_t **conn_ptr) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct epoll_event ev;
    connection_t *conn;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    if (set_nonblocking(sockfd) < 0) {
        perror("set_nonblocking");
        close(sockfd);
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_HOST, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    conn = (connection_t *)malloc(sizeof(connection_t));
    if (!conn) {
        perror("malloc");
        close(sockfd);
        return -1;
    }

    conn->fd = sockfd;
    conn->state = 0;
    conn->bytes_sent = 0;
    conn->bytes_received = 0;
    conn->start_time = time(NULL);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            perror("connect");
            free(conn);
            close(sockfd);
            return -1;
        }
    }

    ev.events = EPOLLOUT | EPOLLIN | EPOLLET;
    ev.data.ptr = conn;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        perror("epoll_ctl: add");
        free(conn);
        close(sockfd);
        return -1;
    }

    *conn_ptr = conn;
    return sockfd;
}

void close_connection(int epoll_fd, connection_t *conn, int success) {
    if (conn->fd >= 0) {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, conn->fd, NULL);
        close(conn->fd);
    }
    
    if (success) {
        successful_requests++;
    } else {
        failed_requests++;
    }
    
    free(conn);
}

int handle_connection(int epoll_fd, connection_t *conn, uint32_t events) {
    char buffer[BUFFER_SIZE];
    ssize_t n;

    if (events & (EPOLLERR | EPOLLHUP)) {
        return -1;
    }

    /* State 0: Connecting */
    if (conn->state == 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0 || error != 0) {
            return -1;
        }
        conn->state = 1;
    }

    /* State 1: Sending request */
    if (conn->state == 1 && (events & EPOLLOUT)) {
        const char *request = request_template;
        size_t request_len = strlen(request);
        
        while (conn->bytes_sent < request_len) {
            n = write(conn->fd, request + conn->bytes_sent, request_len - conn->bytes_sent);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return 0;
                }
                return -1;
            }
            conn->bytes_sent += n;
        }
        conn->state = 2;
    }

    /* State 2: Receiving response */
    if (conn->state == 2 && (events & EPOLLIN)) {
        while (1) {
            n = read(conn->fd, buffer, sizeof(buffer));
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return 0;
                }
                return -1;
            } else if (n == 0) {
                /* Connection closed by server */
                conn->state = 3;
                return 1;
            }
            conn->bytes_received += n;
        }
    }

    return 0;
}

void print_progress(int current, int total) {
    int percentage = (current * 100) / total;
    int bar_width = 50;
    int filled = (percentage * bar_width) / 100;
    
    printf("\r[");
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) printf("=");
        else if (i == filled) printf(">");
        else printf(" ");
    }
    printf("] %d%% (%d/%d) Success: %d Failed: %d", 
           percentage, current, total, successful_requests, failed_requests);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    int epoll_fd;
    struct epoll_event events[MAX_EVENTS];
    int num_connections = 100000;
    int max_concurrent = 5000;
    int active_connections = 0;
    int completed_connections = 0;
    int initiated_connections = 0;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Parse arguments: [method] [total] [concurrent] */
    if (argc > 1) {
        /* Check for method type */
        if (strcmp(argv[1], "get") == 0 || strcmp(argv[1], "GET") == 0) {
            request_template = GET_REQUEST;
            request_mode = "GET /";
        } else if (strcmp(argv[1], "post") == 0 || strcmp(argv[1], "POST") == 0) {
            request_template = UPLOAD_REQUEST;
            request_mode = "POST /uploads";
        } else if (strcmp(argv[1], "cgi") == 0 || strcmp(argv[1], "CGI") == 0) {
            request_template = CGI_REQUEST;
            request_mode = "CGI POST";
        } else {
            /* Assume it's a number (backward compatibility) */
            num_connections = atoi(argv[1]);
            if (num_connections <= 0) {
                fprintf(stderr, "Usage: %s [method] [total] [concurrent]\n", argv[0]);
                fprintf(stderr, "Methods: get, post, cgi\n");
                fprintf(stderr, "Example: %s post 10000 100\n", argv[0]);
                return 1;
            }
        }
    }

    if (argc > 2) {
        num_connections = atoi(argv[2]);
        if (num_connections <= 0) {
            fprintf(stderr, "Invalid number of connections\n");
            return 1;
        }
    }

    if (argc > 3) {
        max_concurrent = atoi(argv[3]);
        if (max_concurrent <= 0 || max_concurrent > 100000) {
            fprintf(stderr, "Invalid max concurrent connections (1-100000)\n");
            return 1;
        }
    }

    printf("╔════════════════════════════════════════╗\n");
    printf("║   WebServ C Stress Test                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf("Target: %s:%d\n", SERVER_HOST, SERVER_PORT);
    printf("Mode: %s\n", request_mode);
    printf("Total requests: %d\n", num_connections);
    printf("Max concurrent: %d\n\n", max_concurrent);

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return 1;
    }

    start_time = time(NULL);
    total_requests = num_connections;

    printf("Starting stress test...\n\n");

    /* Initial batch of connections */
    while (initiated_connections < num_connections && active_connections < max_concurrent && running) {
        connection_t *conn = NULL;
        if (create_connection(epoll_fd, &conn) >= 0) {
            active_connections++;
            initiated_connections++;
        } else {
            failed_requests++;
            usleep(1000); /* Brief delay on failure */
        }
    }

    /* Main event loop */
    while (running && (active_connections > 0 || initiated_connections < num_connections)) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            connection_t *conn = (connection_t *)events[i].data.ptr;
            int result = handle_connection(epoll_fd, conn, events[i].events);
            
            if (result != 0) {
                close_connection(epoll_fd, conn, result > 0);
                active_connections--;
                completed_connections++;
                
                /* Start new connection if needed */
                if (initiated_connections < num_connections && running) {
                    connection_t *new_conn = NULL;
                    if (create_connection(epoll_fd, &new_conn) >= 0) {
                        active_connections++;
                        initiated_connections++;
                        /* Track peak concurrent connections */
                        if (active_connections > peak_concurrent) {
                            peak_concurrent = active_connections;
                        }
                    } else {
                        failed_requests++;
                    }
                }
                
                if (completed_connections % 100 == 0 || completed_connections == total_requests) {
                    print_progress(completed_connections, total_requests);
                }
            }
        }

        /* Timeout check - close connections taking too long */
        if (nfds == 0 && active_connections > 0) {
            /* Could implement timeout logic here if needed */
        }
    }

    time_t end_time = time(NULL);
    double duration = difftime(end_time, start_time);

    printf("\n\n╔════════════════════════════════════════╗\n");
    printf("║   Results                              ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf("Total requests:     %d\n", total_requests);
    printf("Successful:         %d (%.2f%%)\n", successful_requests, 
           (successful_requests * 100.0) / total_requests);
    printf("Failed:             %d (%.2f%%)\n", failed_requests,
           (failed_requests * 100.0) / total_requests);
    printf("Peak concurrent:    %d connections\n", peak_concurrent);
    printf("Duration:           %.2f seconds\n", duration);
    printf("Requests/second:    %.2f\n", total_requests / duration);
    printf("\n");

    if (successful_requests == total_requests) {
        printf("🎉 All requests completed successfully!\n");
    } else if (successful_requests > total_requests * 0.95) {
        printf("✓ Server handled most requests well\n");
    } else {
        printf("⚠ Server had issues handling the load\n");
    }

    close(epoll_fd);
    return (successful_requests == total_requests) ? 0 : 1;
}
