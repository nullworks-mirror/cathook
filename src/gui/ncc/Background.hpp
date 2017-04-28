/*
 * Background.hpp
 *
 *  Created on: Apr 28, 2017
 *      Author: nullifiedcat
 */

#ifndef BACKGROUND_HPP_
#define BACKGROUND_HPP_

#include "Menu.hpp"

extern unsigned char _binary_snowflake_start;

namespace menu { namespace ncc {

class Background : public CBaseWidget {
public:
	struct Snowflake {
		float x, y;
		int show_in { 0 };
		bool dead { false };
		Snowflake* next { nullptr };
		Snowflake* prev { nullptr };
		void Update();
	};
public:
	Background();
	~Background();
	virtual void Draw(int x, int y) override;
	virtual void Update() override;
	void MakeSnowflake();
	void KillSnowflake(Snowflake* flake);
public:
	Texture snowflake_texture;
	Snowflake* list { nullptr };
	Snowflake* list_tail { nullptr };
};

}}

#endif /* BACKGROUND_HPP_ */
