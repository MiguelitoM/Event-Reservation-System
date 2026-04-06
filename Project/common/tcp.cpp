#include "tcp.hpp"
#include <sys/ioctl.h>
int start_tcp(ClientState& st) {
    struct addrinfo hints_tcp;
    struct timeval timeout;
    int errorcode;

    st.tcp_fd = socket(AF_INET,SOCK_STREAM,0);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(st.tcp_fd, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
        perror("Error while setting socket options");
        return 1;
    }
    if (st.tcp_fd == -1) {
        cout << "Something went wrong.\n";
        return 1;
    }

    memset(&hints_tcp, 0, sizeof(hints_tcp));
    hints_tcp.ai_family = AF_INET;
    hints_tcp.ai_socktype= SOCK_STREAM;
    errorcode = getaddrinfo(st.domain,st.port,&hints_tcp,&st.server_addr_tcp);
    if (errorcode) {
        cout << errorcode << "\n";
        return 1;
    }
    int iMode = 0;
    ioctl(st.tcp_fd,FIONBIO,&iMode);   
    if (connect(st.tcp_fd, st.server_addr_tcp->ai_addr, st.server_addr_tcp->ai_addrlen)) {
        cout<< "Server not available\n";
        return 1;
    }
    return 0;
}

int close_tcp(ClientState& st){
    freeaddrinfo(st.server_addr_tcp);
    close(st.tcp_fd);
    return 0;
}

int write_tcp(ClientState& st, string& msg) {
    // Assumes that client is already connected
    if (safe_write(st.tcp_fd, msg) == -1) {
        return 1;
    }
    return 0;
}

int read_tcp(ClientState& st, string& msg) {
    // Assumes that client is already connected
    if (safe_read(st.tcp_fd, msg)) {
        return 1;
    }
    return 0;
}

int send_command_tcp(ClientState& st, string& msg,  string& response) {
    if (start_tcp(st)) {
        return -1;
    }
    if (write_tcp(st, msg)) {
        close_tcp(st);
        return -1;
    }
    if (read_tcp(st, response)) {
        close_tcp(st);
        return -1;
    }
    if (close_tcp(st)) {
        return -1;
    }
    return 0;
}

int send_read_lst(ClientState& st, string& response) {
    response.clear();
    if (start_tcp(st)){
        cout << "unknown error\n";
        return 1;
    }
    string msg = "LST\n";
    if (safe_write(st.tcp_fd, msg)) {
        close_tcp(st);
        return 1;
    }
    string buffer;
    int amount_read = 0;
    int giant_limit = BUFFSIZE*BUFFSIZE;
    while (amount_read < giant_limit) {
        if (safe_read(st.tcp_fd, buffer)) {
            cout << "here?\n";
            close_tcp(st);
            return 1;
        }
        if (buffer.empty()) {
            break;
        }
        amount_read += buffer.size();
        response.append(buffer.data(), buffer.size());
        if (buffer.find('\n') != string::npos) {
            break;
        }
        buffer.clear();
    }
    close_tcp(st);
    return 0;
}