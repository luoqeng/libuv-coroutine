// gcc -o tcp_callback tcp_callback.c -luv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

uv_loop_t *loop;

void alloc_buffer(uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  *buf = uv_buf_init(malloc(size), size);
}

void on_close(uv_handle_t* handle) {
      printf("closed.\n");
}

void on_read(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread < 0) {
        if (nread != UV_EOF)
            fprintf(stderr, "Read error %s\n", uv_err_name(nread));
        uv_close((uv_handle_t*) client, on_close);
        free(buf->base);
        free(client);
        return;
    }

    char *data = (char*) malloc(sizeof(char) * (nread+1));
    data[nread] = '\0';
    strncpy(data, buf->base, nread);

    printf("read.\n%s\n", data);
    free(data);
    free(buf->base);
}


void on_write(uv_write_t* req, int status) {
    if (status) {
        fprintf(stderr, "uv_write error %s\n", uv_err_name(status));
        /*free(req->data)*/
        free(req);
        return;

    }
    printf("wrote.\n");

    uv_read_start((uv_stream_t*) req->handle, alloc_buffer, on_read);
    /*free(req->data)*/
    free(req);
}

void on_connect(uv_connect_t *req, int status) {
    if (status < 0) {
        fprintf(stderr, "connect failed error %s\n", uv_err_name(status));
        free(req);
        return;
    }

    const char* httpget =
        "GET / HTTP/1.0\r\n"
        "Host: www.baidu.com\r\n"
        "Cache-Control: max-age=0\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"
        "\r\n";
    uv_buf_t buf = uv_buf_init((char*)httpget, strlen(httpget));
    uv_write_t *write_req = malloc(sizeof(uv_write_t));
    write_req->data = &buf;
    uv_write(write_req, req->handle, &buf, 1, on_write);

    free(req);
}

void on_resolved(uv_getaddrinfo_t *resolver, int status, struct addrinfo *res) {
    if (status < 0) {
        fprintf(stderr, "getaddrinfo callback error %s\n", uv_err_name(status));
        return;
    }

    char addr[17] = {'\0'};
    uv_ip4_name((struct sockaddr_in*) res->ai_addr, addr, 16);
    printf("%s\n", addr);

    uv_connect_t *connect_req = (uv_connect_t*) malloc(sizeof(uv_connect_t));
    uv_tcp_t *socket = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(loop, socket);

    uv_tcp_connect(connect_req, socket, (const struct sockaddr*) res->ai_addr, on_connect);

    uv_freeaddrinfo(res);
}

int main() {
    loop = uv_default_loop();

    uv_getaddrinfo_t resolver;
    printf("www.baidu.com is... ");
    int r = uv_getaddrinfo(loop, &resolver, on_resolved, "www.baidu.com", "http", NULL);

    if (r) {
        fprintf(stderr, "getaddrinfo call error %s\n", uv_err_name(r));
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);
}
