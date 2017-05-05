/*
 * ipc.h
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#ifdef IPC_ENABLED

#ifndef IPC_H_
#define IPC_H_

#include "beforecheaders.h"
#include "ipcb.hpp"
#include "pthread.h"
#include "aftercheaders.h"

class CatCommand;
class CatVar;

namespace ipc {

namespace commands {

constexpr unsigned execute_client_cmd = 1;
constexpr unsigned set_follow_steamid = 2;
constexpr unsigned execute_client_cmd_long = 3;
constexpr unsigned move_to_vector = 4;
constexpr unsigned stop_moving = 5;
constexpr unsigned start_moving = 6;

}

extern CatCommand connect;
extern CatCommand disconnect;
extern CatCommand exec;
extern CatCommand exec_all;
extern CatCommand lobby;
extern CatVar server_name;

extern pthread_t listener_thread;
constexpr unsigned cathook_magic_number = 0x0DEADCA7;

struct server_data_s {
	unsigned magic_number;
};

struct user_data_s {
	char name[32];
	unsigned friendid;
};

using peer_t = cat_ipc::Peer<server_data_s, user_data_s>;

extern peer_t* peer;

void StoreClientData();
void UpdatePlayerlist();

}

#endif /* IPC_H_ */

#endif
