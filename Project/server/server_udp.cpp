#include "server_udp.hpp"

int parse_client_msg_udp(string msg, int udp_fd, sockaddr_in udp_useraddr,socklen_t addrlen, int verbose) {
    int ret_val = -1;
    string arg;
    vector<string> args;
    stringstream ss(msg);
    while (ss >> arg) {
        args.push_back(arg);
    }

    if (args.size() <= 0) {
        answer_udp("", -1, udp_fd, udp_useraddr, addrlen, verbose);
        return 1;
    }
    if (args[0] == "LIN" || args[0] == "LOU" || args[0] == "UNR" || args[0] == "LME" || args[0] == "LMR") {
        if (args.size() != 3) {
            answer_udp(args[0], -1, udp_fd, udp_useraddr, addrlen, verbose);
            return 1;
        }
        string uid = args[1];
        string password = args[2];
        string response = "";
        if (parse_login_logout(uid, password)) {
            answer_udp(args[0], -1, udp_fd, udp_useraddr, addrlen, verbose);
            return 1;
        }

        if (args[0] == "LIN") {
            ret_val = login_user(uid, password);

        } else if (args[0] == "LOU") {
            ret_val = logout_user(uid, password);
            
        } else if (args[0] == "UNR") {
            ret_val = unregister(uid, password);
        }
        else if (args[0] == "LME") {
            ret_val = myevents(uid, password, response);
        }
        else if (args[0] == "LMR") {
            ret_val = myreservations(uid, password, response);
        }
        answer_udp(args[0], ret_val, udp_fd, udp_useraddr, addrlen, verbose, response, uid);        
    }
    else {
        answer_udp(args[0], -1, udp_fd, udp_useraddr, addrlen, verbose);
        return 1;
    }
    return 0;
}

// Retorna: 0 -> OK, 1 -> user registado, 2 -> wrong password
int login_user(string UID, string PWD){
    int retval = LoginResult::LIN_OK;
    string true_pwd;
    fs::path login_path = logged_in_path(UID);

    if (!does_user_exist(UID)){
        create_user(UID,PWD);
        retval = LoginResult::LIN_REG;
    }

    if (!check_password(UID, PWD)){
        return LoginResult::LIN_NOK;
    }

    ofstream logged_in(login_path);
    logged_in.close();

    return retval;
}

int create_user(string UID, string PWD){
    fs::path user_path = user_dir(UID);
    fs::path reservations_path = user_reservations_dir(UID);
    fs::path events_path = user_events_dir(UID);
    if (!fs::exists(user_path)){
        fs::create_directory(user_path);
    }
    if (!fs::exists(reservations_path)){
        fs::create_directory(reservations_path);
    }
    if (!fs::exists(events_path)){
        fs::create_directory(events_path);
    }
    fs::path passwd_path = password_path(UID);
    ofstream passwd(passwd_path);
    passwd << PWD;
    passwd.close();
    return 0;
}

// 0 -> OK, 1 -> not logged in, 2 -> wrong password, 3 -> user does not exist
int logout_user(string UID, string PWD){
    fs::path login_path = logged_in_path(UID);
    int retval = LogoutResult::LOU_OK;

    if (!does_user_exist(UID)){
        return LogoutResult::LOU_UNR;

    } else if(!is_logged_in(UID)){
        return LogoutResult::LOU_NOK;
    }

    if (!check_password(UID, PWD)){
        return LogoutResult::LOU_WRP;
    }

    fs::remove(login_path);
    return retval;
}

// 0 -> OK, 1 -> not logged in, 2 -> wrong password, 3 -> user does not exist
int unregister(string UID, string PWD){
    fs::path login_path = logged_in_path(UID);
    fs::path passwd_path = password_path(UID);
    int retval = UnregisterResult::UNR_OK;

    if (!does_user_exist(UID)){
        return UnregisterResult::UNR_UNR;

    } else if(!is_logged_in(UID)){
        return UnregisterResult::UNR_NOK;
    }

    if (!check_password(UID, PWD)){
        return UnregisterResult::UNR_WRP;
    }

    fs::remove(login_path);
    fs::remove(passwd_path);
    return retval;
}

int myevents (string uid, string password, string & response){
    if (!does_user_exist(uid)){
        return MyeventsResult::RME_NOK;
    }
    if (!check_password(uid,password)){
        return MyeventsResult::RME_WRP;
    }
    if (!is_logged_in(uid)){
        return MyeventsResult::RME_NLG;
    }

    response = "RME OK";
    bool has_any_events=false;
    set<fs::path> sorted_by_name;
    fs::path user_event_dir= user_events_dir(uid);
    for (auto &entry : fs::directory_iterator(user_event_dir)){
        sorted_by_name.insert(entry.path());
        has_any_events=true;
    }

    if(!has_any_events){
        return MyeventsResult::RME_NOK;
    }
    for (auto &filename : sorted_by_name){
        string eid = filename.string().substr(20);
        int state = get_event_state(eid);

        response += " "+ eid + " " + to_string(state);
    }
    response += "\n";
    return MyeventsResult::RME_OK;
}

int myreservations (string uid, string password, string & response){
    response = "RMR OK";
    if (!does_user_exist(uid)){
        return MyreservationsResult::RMR_NOK;
    }
    if (!check_password(uid,password)){
        return MyreservationsResult::RMR_WRP;
    }
    if (!is_logged_in(uid)){
        return MyreservationsResult::RMR_NLG;
    }

    bool has_any_reservations = false;
    set<fs::path> sorted_by_name;
    fs::path user_reservations_path = user_reservations_dir(uid);
    for (auto &entry : fs::directory_iterator(user_reservations_path)){
        sorted_by_name.insert(entry.path());
        has_any_reservations=true;
    }
    if(!has_any_reservations){
        return MyreservationsResult::RMR_NOK;
    }
    int reservations = 0;
    set<fs::path>::reverse_iterator rit;
    for (rit = sorted_by_name.rbegin(); rit != sorted_by_name.rend(); rit++){
        string eid = rit->string().substr(46);
        string year = rit->string().substr(26,4);
        string month = rit->string().substr(31,2);
        string day = rit->string().substr(34,2);
        string hour = rit->string().substr(37,2);
        string minute = rit->string().substr(40,2);
        string second = rit->string().substr(43,2);
        ifstream file(*rit);
        string amount;
        while (file >> amount) {
            string event_details = " "+ eid + " " + day + "-" + month + "-" + year + " " + hour + ":" + minute + ":"+ second + " " + amount;
            response.append(event_details.data(), event_details.size());
            reservations++;
            if (reservations == 50){
                break;
            }
        }  
        if (reservations == 50){
            break;
        }  

    }
    response += "\n";
    return MyreservationsResult::RMR_OK;
}

int answer_udp(string command, int ret_val, int udp_fd, struct sockaddr_in udp_useraddr,
                socklen_t addrlen, int verbose, string msg, string uid) {
    string response = "ERR\n";
    string answer = "";

    if (command == "LIN") {
        if (ret_val == LoginResult::LIN_ERR) {
            response = "RLI ERR\n";
        } else if (ret_val == LoginResult::LIN_OK) {
            response = "RLI OK\n";
        }
        else if (ret_val == LoginResult::LIN_REG) {
            response = "RLI REG\n";
        }
        else if (ret_val == LoginResult::LIN_NOK) {
            response = "RLI NOK\n";
        }
    }

    else if (command == "LOU") {
        if (ret_val == LogoutResult::LOU_ERR) {
            response = "RLO ERR\n";
        } else if (ret_val == LogoutResult::LOU_OK) {
            response = "RLO OK\n";
        } else if (ret_val == LogoutResult::LOU_NOK) {
            response = "RLO NOK\n";
        } else if (ret_val == LogoutResult::LOU_WRP) {
            response = "RLO WRP\n";
        } else if (ret_val == LogoutResult::LOU_UNR) {
            response = "RLO UNR\n";
        }
    }
    else if (command == "UNR") {
        if (ret_val == UnregisterResult::UNR_ERR) {
            response = "RUR ERR\n";
        } else if (ret_val == UnregisterResult::UNR_OK) {
            response = "RUR OK\n";
        } else if (ret_val == UnregisterResult::UNR_NOK) {
            response = "RUR NOK\n";
        } else if (ret_val == UnregisterResult::UNR_WRP) {
            response = "RUR WRP\n";
        } else if (ret_val == UnregisterResult::UNR_UNR) {
            response = "RUR UNR\n";
        }
    }
    else if (command == "LME") {
        if (ret_val == MyeventsResult::RME_ERR) {
            response = "RME ERR\n";
        } else if (ret_val == MyeventsResult::RME_OK) {
            response = msg;
            answer = "RME OK\n";
        } else if (ret_val == MyeventsResult::RME_NOK) {
            response = "RME NOK\n";
        } else if (ret_val == MyeventsResult::RME_WRP) {
            response = "RME WRP\n";
        } else if (ret_val == MyeventsResult::RME_NLG) {
            response = "RME NLG\n";
        }
    }
    else if (command == "LMR") {
        if (ret_val == MyreservationsResult::RMR_ERR) {
            response = "RMR ERR\n";
        } else if (ret_val == MyreservationsResult::RMR_OK) {
            response = msg;
            answer = "RMR OK\n";
        } else if (ret_val == MyreservationsResult::RMR_NOK) {
            response = "RMR NOK\n";
        } else if (ret_val == MyreservationsResult::RMR_WRP) {
            response = "RMR WRP\n";
        } else if (ret_val == MyreservationsResult::RMR_NLG) {
            response = "RMR NLG\n";
        }
    }
    int n = sendto(udp_fd, response.c_str(), response.size(), 0, (struct sockaddr*) &udp_useraddr, addrlen);

    if (n == -1) {
        return -1;
    }

    if (uid == "") {
        uid = "Unknown";
    }
    if (answer == "") {
        answer = response;
    }
    answer.pop_back();

    if (verbose) {
        cout << "Answer: " << answer << ", UID: " << uid << ", IP: " 
        << inet_ntoa(udp_useraddr.sin_addr) << ", port: " 
        << ntohs(udp_useraddr.sin_port) << "\n";
    }
    return 0;
}

void log_udp_request(const string& msg, const sockaddr_in& addr) {
    string cmd;
    string uid = "Unknown";
    stringstream ss(msg);
    ss >> cmd;

    string pretty;

    if (cmd == "LIN") {
        pretty = "Login";
    } else if (cmd == "LOU") {
        pretty = "Logout";
    } else if (cmd == "UNR") {
        pretty = "Unregister";
    } else if (cmd == "LME") {
        pretty = "MyEvents";
    } else if (cmd == "LMR") {
        pretty = "MyReservations";
    } else {
        pretty = "Unknown";
    }

    string maybe_uid;
    if (ss >> maybe_uid) {
        if (!parse_uid(maybe_uid)) {
            uid = maybe_uid;
        }
    }
    
    cout << "Request: " << pretty << ", UID: " << uid << ", IP: " 
        << inet_ntoa(addr.sin_addr) << ", port: " 
        << ntohs(addr.sin_port) << "\n";
}