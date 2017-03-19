/*
 * entitywrap.h
 *
 *  Created on: Mar 18, 2017
 *      Author: nullifiedcat
 */

#ifndef ENTITYWRAP_H_
#define ENTITYWRAP_H_

class IClientEntity;

class EntityWrapper {
public:
	EntityWrapper(int idx);

	bool good() const;
	IClientEntity* operator->() const;

	const int idx;
};

#endif /* ENTITYWRAP_H_ */
