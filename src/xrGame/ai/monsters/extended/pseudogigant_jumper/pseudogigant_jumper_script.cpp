///////////////////////////////////////////////////////////////////////////
//  Базовый класс: Псевдогигант
//	Мутант: Прыгающий псевдогигант
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "pch_script.h"
#include "../../pseudogigant/pseudogigant.h"
#include "pseudogigant_jumper.h"

using namespace luabind;

#pragma optimize("s",on)
void CPseudogigantJumper::script_register(lua_State *L) 
{
	module(L) 
	[
		class_<CPseudogigantJumper,CGameObject>("CPseudoGigantJumper")
		.def(constructor<>())
	];
}
