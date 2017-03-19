/*
 * ipc.h
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#ifndef IPC_H_
#define IPC_H_

#include "ipcb.hpp"

class CatCommand;
class CatVar;

namespace ipc {

extern CatCommand connect;
extern CatCommand disconnect;
extern CatCommand exec;
extern CatCommand exec_all;
extern CatVar server_name;

using peer_t = cat_ipc::Peer<server_data_s, user_data_s>;

struct server_data_s {
	bool dummy;
};

struct user_data_s {
	char name[32];
	unsigned friendid;
};

extern peer_t* peer;

void StoreClientData();

}

#endif /* IPC_H_ */
