/*
 * ipc.h
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#ifndef IPC_H_
#define IPC_H_

#include "ipcb.hpp"
#include "pthread.h"

class CatCommand;
class CatVar;

namespace ipc {

extern CatCommand connect;
extern CatCommand disconnect;
extern CatCommand exec;
extern CatCommand exec_all;
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

}

#endif /* IPC_H_ */
