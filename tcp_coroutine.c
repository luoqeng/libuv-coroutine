// gcc -o tcp_coroutine acosw.S aco.c tcp_coroutine.c -luv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>

#include "aco.h"


typedef struct {
    int status;
    void *data;
    aco_t* co;
} await_state_t;


uv_loop_t *loop;
aco_t* co;

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  *buf = uv_buf_init(malloc(size), size);
}

void on_close(uv_handle_t* handle) {
      printf("closed.\n");
      free(handle);
}

void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    await_state_t *state = (await_state_t*)client->data;
    if (nread < 0) {
        if (nread != UV_EOF) fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
        free(buf->base);

        state->status = nread;
        co = state->co;
        return;
    }

    int len = state->status + sizeof(char) * (nread+1);
    state->data = realloc(state->data, len);
    ((char *)state->data)[state->status-1] = '\0';
    state->status = len;
    strncat(state->data, buf->base, nread);

    /*char *data = (char*) malloc(sizeof(char) * (nread+1));*/
    /*data[nread] = '\0';*/
    /*strncpy(data, buf->base, nread);*/

    free(buf->base);
}

int co_read_start(await_state_t *state, uv_stream_t *handle) {
    state->co = aco_get_co();
    handle->data=state;
    int ret = uv_read_start(handle, alloc_buffer, on_read);
    if (ret) {
        return ret;
    }
    aco_yield();
    return state->status;
}

void on_write(uv_write_t* req, int status) {
    await_state_t *state = (await_state_t*)req->data;
    state->status = status;
    free(req);
    co = state->co;
}

int co_write(await_state_t *state, uv_stream_t *handle, const uv_buf_t bufs[], unsigned int nbufs) {
    state->co = aco_get_co();
    state->data = (void*)bufs;
    uv_write_t *write_req = malloc(sizeof(uv_write_t));
    write_req->data = state;
    int ret = uv_write(write_req, handle, bufs, nbufs, on_write);
    if (ret) {
        free(write_req);
        return ret;
    }
    aco_yield();

    return state->status;
}

void on_connect(uv_connect_t *req, int status) {
    await_state_t *state = (await_state_t*)req->data;
    state->status = status;
    state->data = req->handle;
    free(req);
    co = state->co;
}

int co_tcp_connect(await_state_t *state, const struct sockaddr *dest) {
    char addr[17] = {'\0'};
    uv_ip4_name((struct sockaddr_in*) dest, addr, 16);
    printf("%s\n", addr);

    state->co = aco_get_co();
    uv_connect_t *connect_req = (uv_connect_t*) malloc(sizeof(uv_connect_t));
    connect_req->data = state;
    uv_tcp_t *socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);
    int r = uv_tcp_connect(connect_req, socket, (const struct sockaddr*) dest, on_connect);
    if (r) {
        free(connect_req);
        free(socket);
        return r;
    }
    aco_yield();

    return state->status;
}

void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
    await_state_t *state = (await_state_t*)resolver->data;
    state->status = status;
    state->data = res;
    co = state->co;
}

int co_getaddrinfo(await_state_t *state, const char *node, const char *service, const struct addrinfo *hints) {
    printf("%s is... ", node);

    state->co = aco_get_co();
    uv_getaddrinfo_t *resolver = (uv_getaddrinfo_t*) malloc(sizeof(uv_getaddrinfo_t));
    resolver->data = state;
    int r = uv_getaddrinfo(loop, resolver, on_resolved, node, service, hints);
    if (r) {
        free(resolver);
        return r;
    }
    aco_yield();

    free(resolver);
    return state->status;
}

void http_get() {
    await_state_t *state = (await_state_t*) malloc(sizeof(await_state_t));
    memset(state, 0, sizeof(await_state_t));
    int status = co_getaddrinfo(state, "www.baidu.com", "http", NULL);
    struct addrinfo *addr = (struct addrinfo*)state->data;
    if (status < 0) {
        fprintf(stderr, "co_getaddrinfo error %s\n", uv_err_name(status));
        if (addr) uv_freeaddrinfo(addr);
        free(state);
        return;
    }

    memset(state, 0, sizeof(await_state_t));
    status = co_tcp_connect(state, addr->ai_addr);
    uv_freeaddrinfo(addr);
    uv_stream_t *handle = (uv_stream_t*)state->data;
    if (status < 0) {
        fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
        if (handle) free(handle);
        free(state);
        return;
    }

    const char* httpget =
        "GET / HTTP/1.0\r\n"
        "Host: www.baidu.com\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
        "\r\n";
    uv_buf_t buffer = uv_buf_init((char*)httpget, strlen(httpget));
    memset(state, 0, sizeof(await_state_t));
    status = co_write(state, handle, &buffer, 1);
    if (status) {
        fprintf(stderr, "co_write error %s\n", uv_err_name(status));
        return;
    }
    printf("wrote.\n");

    memset(state, 0, sizeof(await_state_t));
    status = co_read_start(state, handle);
    char *read_data = (char*)state->data;
    if (status) {
        fprintf(stderr, "co_read_start error %s\n", uv_err_name(status));
        return;
    }
    printf("read.\n%s\n", read_data);
    free(read_data);

    aco_exit();
}

int main() {
    loop = uv_default_loop();

    aco_thread_init(NULL);
    aco_t* main_co = aco_create(NULL, NULL, 0, NULL, NULL);
    aco_share_stack_t* sstk = aco_share_stack_new(0);
    co = aco_create(main_co, sstk, 0, http_get, NULL);
    aco_resume(co);
    co = NULL;

    for (;;) {
        uv_run(loop, UV_RUN_NOWAIT);
        if (co) {
            if(co->is_end) {
                break;
            }
            aco_resume(co);
            co = NULL;
        }
    }

    aco_destroy(co);
    co = NULL;
    aco_share_stack_destroy(sstk);
    sstk = NULL;
    aco_destroy(main_co);
    main_co = NULL;

    uv_loop_close(loop);

    return 0;
}
