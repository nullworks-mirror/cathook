/*
 * List.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#ifndef LIST_HPP_
#define LIST_HPP_

#include "../CBaseContainer.h"
#include "../../common.h"

class Item;

namespace menu { namespace ncc {

class List : public CBaseContainer {
public:
	List(std::string title);
	List();

	void FillWithCatVars(std::vector<std::string> vec);
	void FillWithCatVars(std::vector<CatVar*> vec);
	void OpenSublist(List* sublist, int dy);
	bool ShouldClose();

	static List* FromString(const std::string& string);

	//virtual IWidget* ChildByPoint(int x, int y) override;
	inline virtual void SortByZIndex() override {};
	virtual void Show() override;
	virtual void OnKeyPress(ButtonCode_t key, bool repeat) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;
	virtual void Draw(int x, int y) override;
	virtual void Update() override;
	virtual void MoveChildren() override;
	virtual void SetParent(IWidget* parent) override;
public:
	List* root_list { nullptr };
	bool got_mouse { false };
	List* open_sublist { nullptr };
	std::string title;
};

}}

#endif /* LIST_HPP_ */
