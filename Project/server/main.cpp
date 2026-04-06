#include "../common/io.hpp"
#include "../common/tcp.hpp"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <filesystem> 
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "CreState.hpp"
#include "utils.hpp"
#include "server_udp.hpp"
#include "server_tcp.hpp"

using namespace std;
namespace fs = std::filesystem;
#define GN 74
#define MAX_CLIENTS 512
bool verbose;

// The most important function. Handles most of the pseudo-concurrency logic. See the broader view in the readme.txt
void select_loop(fd_set& inputs, int& max_fd, int ufd, int tfd) {
    fd_set testfds;
    struct timeval timeout;
    int out_fds, n, errcode;
    char msg[BUFFSIZE];

    struct sockaddr_in udp_useraddr, tcp_useraddr;
    socklen_t udp_addrlen, tcp_addrlen;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    map<int, string> tcp_buffers; // Guarda as mensagens que só serão realmente processadas depois de terem "\n". 
                                        // Não conseguimos garantir ler a mensagem inteira numa iteração do select.
    // Maintains the state of file descriptors associated with Create commands
    map <int, CreState> states;

    // Maintains a registry of the beginning of every TCP session for each file descriptor
    map<int, time_t> connection_start;

    // Maintains the socket information for each file descriptor
    map<int, sockaddr_in> tcp_clients;

    //currently active clients
    int active_clients = 0;

    int retval;

    // Main iteration loop
    while(true) {

        //Guarantees an empty buffer
        memset(msg,0,sizeof(msg));
        testfds = inputs; // Reload mask
        memset((void *)&timeout,0,sizeof(timeout));
        timeout.tv_sec=10;

        out_fds = select(max_fd+1,&testfds,(fd_set *)NULL,(fd_set *)NULL,(struct timeval *) &timeout);

        if (out_fds < 0) {
            return;
        }

        if (out_fds == 0) {
        }

        if(FD_ISSET(0, &testfds))
            {
                // Some kind of parser for server here
                // Maybe clean command for server would be appreciated?
            }
        
        // UDP server section
        if(FD_ISSET(ufd,&testfds)) {
            udp_addrlen = sizeof(udp_useraddr);
            n = recvfrom(ufd, msg, sizeof(msg)-1, 0, (struct sockaddr *)&udp_useraddr,&udp_addrlen);
            if(n > 0) {
                msg[n] = '\0';
                errcode = getnameinfo((struct sockaddr *)&udp_useraddr, udp_addrlen, host, sizeof host, service, sizeof service, 0);
                if(errcode == 0) {
                    string msg_string = msg;
                    if (verbose) {
                        log_udp_request(msg_string, udp_useraddr);
                    }
                    parse_client_msg_udp(msg_string,ufd,udp_useraddr,udp_addrlen, verbose);
                }
            }
        }
        
        // TCP server section
        if(FD_ISSET(tfd,&testfds)) {
            tcp_addrlen = sizeof(tcp_useraddr);
            
            int current_fd = accept(tfd, (struct sockaddr *)&tcp_useraddr, &tcp_addrlen);
            if (current_fd > 0) {
                if (verbose){
                cout << "New TCP connection, IP: " << inet_ntoa(tcp_useraddr.sin_addr) 
                    << ", port: " << ntohs(tcp_useraddr.sin_port) << "\n";
                }
                
                //Cleans up any timed out client beffore attempting accepting a new client
                for (int fd = 0; fd <= max_fd; ++fd) {
                    if (fd == 0 || fd == ufd || fd == tfd) {
                        continue;
                    }
                    if (time_out(fd, connection_start)) {
                        if (verbose) {
                            sockaddr_in addr = tcp_clients[fd];
                            cout << "TCP timeout. IP: " << inet_ntoa(addr.sin_addr) << ", port: " << ntohs(addr.sin_port) << "\n";
                        }
                        disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                        active_clients--;
                        continue;
                    }
                }

                if (active_clients >= MAX_CLIENTS) {
                    if (verbose) {
                        cout << "TCP connection refused. Server is at maximum capacity. IP: " 
                        << inet_ntoa(tcp_useraddr.sin_addr) << ", port: " << ntohs(tcp_useraddr.sin_port) << "\n";
                    }
                    close(current_fd);
                    continue;
                }
                // Force the read to be non blocking
                int iMode = 1;
                ioctl(current_fd,FIONBIO,&iMode);
                FD_SET(current_fd, &inputs);
                if (current_fd > max_fd) {
                    max_fd = current_fd;
                }

                connection_start[current_fd] = time(nullptr);
                tcp_clients[current_fd] = tcp_useraddr;
                active_clients++;
            }
        }

        // searching for the active file descriptors from which to read
        for (int fd = 0; fd <= max_fd; ++fd) {
            if (fd == 0 || fd == ufd || fd == tfd) {
                continue;
            }

            if (!FD_ISSET(fd, &testfds)) {
                continue;
            }
            // The main read. 
            //errno is reset
            errno = 0;
            n = read(fd,  msg, sizeof(msg));
            string &buffer = tcp_buffers[fd];

            if ((n < 0 && errno != EINTR && errno != 0 && errno != EAGAIN)) {
                // If there is a read failure, kill the connection to the client
                answer_ERR(fd, verbose, tcp_clients[fd]);
                disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                active_clients--;
                continue;
            }
            else if (n == 0){
                // If there was activity on a file descriptor and none is reported, without it being a normal system interrupt, it is because 
                // something is wrong in the connection
                if (verbose) {
                    sockaddr_in addr = tcp_clients[fd];
                    cout << "TCP client disconnected. IP: " << inet_ntoa(addr.sin_addr) << ", port: " << ntohs(addr.sin_port) << "\n";
                }
                // Não faz sentido responder ERR. Cliente já não está a ouvir
                disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                active_clients--;
            }
            else if (n > 0){
                
                //verify that it is not a create

                if(states[fd].header_complete != true){

                    //add the message to the buffer
                    buffer.append(msg, n);
                    retval = try_to_parse_create(fd, buffer, states[fd], verbose, tcp_clients[fd]);
                    if (states[fd].header_complete == true && verbose) {
                        log_tcp_request(buffer, tcp_useraddr);
                    }
                    if (retval == 0){
                        continue;
                    } else if (retval == -1) {
                        disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                        active_clients--;
                    } else {
                        if (buffer.size() > BUFFSIZE) {
                            answer_ERR(fd, verbose,tcp_clients[fd]);
                            disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                            active_clients--;
                        }

                        size_t pos = buffer.find('\n');
                        if (pos != string::npos) {;
                            string command = buffer.substr(0, pos);
                            if (verbose) {
                                log_tcp_request(command, tcp_useraddr);
                            }
                            parse_client_msg_tcp(fd, command, verbose, tcp_clients[fd]);
                            disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                            active_clients--;
                        }
                    }
                } 
                else {
                    int retval = write_event(states[fd], msg, n);
                    if (retval == -1) {
                        answer_create(CreateResult::CRE_NOK,fd,states[fd],  verbose,  tcp_clients[fd]);
                        disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                        active_clients--;
                    } else if(retval == 1){
                        answer_create(CreateResult::CRE_OK,fd,states[fd],  verbose,  tcp_clients[fd]);
                        disconnect_tcp(fd, states, inputs, tcp_buffers, connection_start, tcp_clients);
                        active_clients--;
                    }
                }
            }
        }
    }
}

// This function puts in place the basic networking stack before starting up the select loop proper
int server_select(char* port){
    fd_set inputs;
    int errcode;

    // socket variables
    struct addrinfo hints_udp, *res_udp, hints_tcp, *res_tcp;;
    int ufd, tfd, max_fd;

    // UDP SERVER SECTION
    memset(&hints_udp, 0, sizeof(hints_udp));
    hints_udp.ai_family = AF_INET;
    hints_udp.ai_socktype = SOCK_DGRAM;
    hints_udp.ai_flags = AI_PASSIVE|AI_NUMERICSERV;

    if ((errcode = getaddrinfo(NULL, port, &hints_udp, &res_udp)) != 0){
        return 1;
    }
  

    ufd = socket(res_udp->ai_family, res_udp->ai_socktype, res_udp->ai_protocol);
    if (ufd==-1) {
        return 1;
    }

    if (::bind(ufd, res_udp->ai_addr, res_udp->ai_addrlen) == -1) {
        return 1;
    }
    if (res_udp != NULL) {
        freeaddrinfo(res_udp);
    }

    // TCP SERVER SECTION
    memset(&hints_tcp, 0, sizeof(hints_tcp));
    hints_tcp.ai_family = AF_INET;
    hints_tcp.ai_socktype = SOCK_STREAM;
    hints_tcp.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    if ((errcode = getaddrinfo(NULL, port, &hints_tcp, &res_tcp)) != 0) {
        return 1;
    }

    tfd = socket(res_tcp->ai_family, res_tcp->ai_socktype, res_tcp->ai_protocol);
    if (tfd == -1) {
        return 1;
    }

    if (::bind(tfd, res_tcp->ai_addr, res_tcp->ai_addrlen) == -1){
        return 1;    
    }

    if (listen(tfd, SOMAXCONN) == -1){
        return 1;       
    }

    freeaddrinfo(res_tcp);
    FD_ZERO(&inputs); // Clear input mask
    FD_SET(0,&inputs); // Set standard input channel off
    FD_SET(ufd,&inputs); // Set UDP channel on
    FD_SET(tfd, &inputs);
    max_fd=ufd;
    if (tfd > max_fd) {
        max_fd = tfd;
    }

    select_loop(inputs, max_fd, ufd, tfd);
    return 0;
}

// The basic initial setup and handling of command line arguments

int main(int argc, char *argv[]){
    int port = 58000 + GN;
    char port_str[7];
    snprintf(port_str, sizeof(port_str), "%d",port);
    verbose = false;

    // This is done to prevent our forked process from becoming a "zombie"
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    // Basic directory setup
    if (!fs::exists(fs::path(USER_PATH))){  
        fs::create_directory(fs::path(USER_PATH));
    }
    if (!fs::exists(fs::path(EVENT_PATH))){
        fs::create_directory(fs::path(EVENT_PATH));
    }

    fs::path event_count = string(EVENT_PATH)+string("/")+string(EVENT_COUNT);
    if (!fs::exists(event_count)){
        ofstream event_count_file(event_count);
        event_count_file << 0;
        event_count_file.close();
    }
    
    //Argument handling
    if (argc > 4) {
        cout << "Too many arguments\n";
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        
        if (arg == "-p" && i+1 < argc) {
            snprintf(port_str, sizeof(port_str), "%s",argv[++i]); 
        }
        else if (arg == "-v") {
            verbose = true;
        }
    }

    cout << "port: ";
    cout.write(port_str,7);
    cout << endl;
    cout << "verbose: " << verbose << "\n";

    if (server_select(port_str)) {
        return 1;
    }
    return 0;
}