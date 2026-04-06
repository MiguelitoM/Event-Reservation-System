#include "udp.hpp"

int start_udp(ClientState& st, char* domain, char* s_port){
    struct addrinfo hints_udp;
    struct timeval timeout;
    int errorcode;

    st.udp_fd = socket(AF_INET,SOCK_DGRAM,0);
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(st.udp_fd, SOL_SOCKET, SO_RCVTIMEO,&timeout,sizeof(timeout)) < 0) {
        perror("Error while setting socket options");
        return 1;
    }
    if (st.udp_fd == -1) {
        cout << "Something went wrong.\n";
        return 1;
    }

    memset(&hints_udp, 0, sizeof(hints_udp));
    hints_udp.ai_family = AF_INET;
    hints_udp.ai_socktype= SOCK_DGRAM;
    errorcode = getaddrinfo(domain,s_port,&hints_udp,&st.server_addr_udp);
    if (errorcode) {
        return 1;
    }
    return 0;
}

int send_command_udp(ClientState& st, string& msg, char* response) {
    if (sendto(st.udp_fd, msg.c_str(), msg.size(), 0, st.server_addr_udp->ai_addr, st.server_addr_udp->ai_addrlen) == -1) {
        cout << "Couldn't send to server\n";
        return -1;
    }
    if (recvfrom(st.udp_fd, response, RESPONSE_SIZE, 0, st.server_addr_udp->ai_addr, &st.server_addr_udp->ai_addrlen) == -1){
        cout << "No answer from server\n";
        return -1;
    }
    return 0;
}

int send_command_udp_myevents(ClientState& st, string& msg, char* response) {
    if (sendto(st.udp_fd, msg.c_str(), msg.size(), 0, st.server_addr_udp->ai_addr, st.server_addr_udp->ai_addrlen) == -1) {
        cout << "Couldn't send to server\n";
        return -1;
    }
    if (recvfrom(st.udp_fd, response, RESPONSE_SIZE_MYEVENTS, 0, st.server_addr_udp->ai_addr, &st.server_addr_udp->ai_addrlen) == -1){
        cout << "No answer from server\n";
        return -1;
    }
    return 0;
}

int send_command_udp_myreservations(ClientState& st, string& msg, char* response) {
    if (sendto(st.udp_fd, msg.c_str(), msg.size(), 0, st.server_addr_udp->ai_addr, st.server_addr_udp->ai_addrlen) == -1) {
        cout << "Couldn't send to server\n";
        return -1;
    }
    if (recvfrom(st.udp_fd, response, RESPONSE_SIZE_MYRESERVATIONS, 0, st.server_addr_udp->ai_addr, &st.server_addr_udp->ai_addrlen) == -1){
        cout << "No answer from server\n";
        return -1;
    }
    return 0;
}